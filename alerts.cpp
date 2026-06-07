#include "monitor.h"

static bool buzzerOn = false, telegramOn = false;

static bool inRange(int i) {
  float v = readings[i].value;
  switch (i) {
    case TEMP:  return v >= settings.tempCool && v <= settings.tempWarm;
    case HUMID: return v >= settings.humidDry && v <= settings.humidHumid;
    case CO2:   return v >= settings.co2Low && v <= settings.co2High;
    default:    return true;
  }
}

static bool dark() {
  return readings[LIGHT].value < settings.nightLevel;
}

void alertsBegin() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

const char* rangeStatus(int i) {
  float v = readings[i].value;
  switch (i) {
    case TEMP:
      if (v < settings.tempCool) return "too cold";
      if (v > settings.tempWarm) return "too warm";
      return "fine";
    case HUMID:
      if (v < settings.humidDry) return "too dry";
      if (v > settings.humidHumid) return "too humid";
      return "fine";
    case CO2:
      if (v < settings.co2Low) return "too low";
      if (v > settings.co2High) return "too high";
      return "fine";
    default:
      return v < settings.nightLevel ? "night" : "day";
  }
}

String formatValue(int i) {
  Reading& r = readings[i];
  String s = String(r.value, r.decimals);
  if (r.unit[0]) s += " " + String(r.unit);
  return s;
}

void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}

static void beep() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(120);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

static String alertReason(const bool* watch) {
  String s = "";
  for (int i = 0; i < READING_COUNT; i++) {
    if (i == LIGHT || !watch[i] || inRange(i)) continue;
    if (s.length()) s += ", ";
    s += String(readings[i].label) + " " + rangeStatus(i) + " (" + formatValue(i) + ")";
  }
  return s;
}

String notifyReason() {
  bool any[READING_COUNT];
  for (int i = 0; i < READING_COUNT; i++) any[i] = settings.buzzer[i] || settings.telegram[i];
  return alertReason(any);
}

void updateAlerts() {
  bool out = !inRange(TEMP) || !inRange(HUMID) || !inRange(CO2);
  if (out)         setLED(255, 0, 0);
  else if (dark()) setLED(0, 0, 255);
  else             setLED(0, 255, 0);

  String buzz = alertReason(settings.buzzer);
  if (buzz.length() && !buzzerOn) beep();
  buzzerOn = buzz.length() > 0;

  String tele = alertReason(settings.telegram);
  if (tele.length() && !telegramOn) sendTelegram("Notification: " + tele + ".");
  telegramOn = tele.length() > 0;
}