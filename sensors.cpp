#include "monitor.h"
#include <DHT.h>

static DHT dht(DHT_PIN, DHT22);

static float rsFromRaw(int raw) {
  return RL_VALUE * (1023.0 / (float)raw - 1.0);
}

static float ppmFromRaw(int raw) {
  float ratio = rsFromRaw(raw) / settings.r0;
  float ppm = 116.6020682 * pow(ratio, -2.769034857);
  return constrain(ppm, 0, 5000);
}

void sensorsBegin() {
  dht.begin();
}

void defineReading(int i, const char* key, const char* label, const char* unit, byte decimals) {
  Reading& r = readings[i];
  r.key = key;
  r.label = label;
  r.unit = unit;
  r.decimals = decimals;
  r.value = 0;
}

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) readings[TEMP].value = t;
  if (!isnan(h)) readings[HUMID].value = h;
  readings[CO2].value = ppmFromRaw(analogRead(MQ135_PIN));
  readings[LIGHT].value = analogRead(LDR_PIN);
}

void calibrate() {
  showMessage("Calibrating");
  float total = 0;
  for (int i = 0; i < 50; i++) {
    total += rsFromRaw(analogRead(MQ135_PIN));
    delay(100);
  }
  float rsClean = total / 50.0;
  float cleanRatio = pow(CO2_BASELINE / 116.6020682, 1.0 / -2.769034857);
  settings.r0 = rsClean / cleanRatio;
  saveState();
  Serial.print("Rs(clean) = "); Serial.print(rsClean);
  Serial.print("  R0 = "); Serial.println(settings.r0);
}