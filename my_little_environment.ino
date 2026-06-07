#include "monitor.h"

Reading readings[READING_COUNT];

Series series[SERIES_COUNT] = {
  { 60, 60000UL },
  { 96, 900000UL },
  { 84, 7200000UL },
};

Settings settings = {
  18, 27,
  35, 60,
  800, 1200,
  200,
  76.63f,
  { false, false, true, false },
  { false, false, true, false },
};

unsigned long bootEpoch = 0;

static const unsigned long SAVE_EVERY = 1800000UL;
static unsigned long lastRead = 0, lastSave = 0, lastTelegram = 0, lastSheets = 0, lastClockSync = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  alertsBegin();
  sensorsBegin();
  displayBegin();
  setLED(0, 0, 255);

  defineReading(TEMP,  "temp",     "Temperature",    "C",   1);
  defineReading(HUMID, "humidity", "Humidity",       "%",   1);
  defineReading(CO2,   "co2",      "Carbon dioxide", "PPM", 0);
  defineReading(LIGHT, "light",    "Light",          "",    0);

  showMessage("Starting");
  wifiConnect();
  webBegin();
  syncClock();
  Serial.print("Dashboard: http://");
  Serial.println(ipString());

  bool restored = loadState();

  int warmupSeconds = restored ? 20 : 180;
  for (int i = warmupSeconds; i > 0; i--) {
    char buf[24];
    snprintf(buf, sizeof(buf), "Warming up %ds", i);
    showMessage(buf);
    delay(1000);
  }
  if (!restored) calibrate();

  readSensors();
  if (!restored) for (int s = 0; s < SERIES_COUNT; s++) recordSample(s);

  unsigned long now = millis();
  for (int s = 0; s < SERIES_COUNT; s++) series[s].lastSampleMs = now;
  lastSave = lastClockSync = now;

  sendTelegram("Online. Dashboard at " + ipString());
  setLED(0, 255, 0);
  showMessage("Ready");
  delay(1000);
}

void loop() {
  unsigned long now = millis();

  if (now - lastRead >= 2000) {
    lastRead = now;
    readSensors();
    drawScreen();
    updateAlerts();
  }

  for (int s = 0; s < SERIES_COUNT; s++) {
    if (now - series[s].lastSampleMs >= series[s].intervalMs) {
      series[s].lastSampleMs = now;
      recordSample(s);
    }
  }

  if (now - lastSave >= SAVE_EVERY) { lastSave = now; saveState(); }

  if (now - lastTelegram >= 5000)     { lastTelegram = now; checkTelegram(); }
  if (now - lastSheets   >= 300000UL) { lastSheets   = now; logToSheets();   }

  unsigned long syncEvery = bootEpoch ? 86400000UL : 15000UL;
  if (now - lastClockSync >= syncEvery) { lastClockSync = now; syncClock(); }

  handleWeb();
}
