#include "monitor.h"
#include <EEPROM.h>
#include <WiFiS3.h>

static const uint32_t STATE_SIGNATURE = 0x454E5636UL;

unsigned long clockNow() {
  return bootEpoch ? bootEpoch + millis() / 1000UL : 0;
}

void syncClock() {
  for (int i = 0; i < 12; i++) {
    unsigned long t = WiFi.getTime();
    if (t > 1000000000UL) { bootEpoch = t - millis() / 1000UL; return; }
    delay(500);
  }
}

String clockHHMM() {
  unsigned long t = clockNow();
  if (!t) return "--:--";
  long s = (((long)t + TZ_OFFSET_HOURS * 3600L) % 86400 + 86400) % 86400;
  char b[6];
  snprintf(b, sizeof(b), "%02ld:%02ld", s / 3600, (s % 3600) / 60);
  return String(b);
}

void recordSample(int s) {
  uint32_t t = clockNow();
  if (!t) return;
  Series& sr = series[s];
  sr.times[sr.head] = t;
  for (int r = 0; r < READING_COUNT; r++) sr.values[sr.head][r] = readings[r].value;
  sr.head = (sr.head + 1) % sr.capacity;
  if (sr.fill < sr.capacity) sr.fill++;
}

int pickSeries(unsigned long from) {
  unsigned long now = clockNow();
  unsigned long span = now > from ? now - from : 0;
  if (span <= 3600UL + 300UL)   return HOUR;
  if (span <= 86400UL + 1800UL) return DAY;
  return WEEK;
}

template <class T> static void eePut(int& a, const T& v) { EEPROM.put(a, v); a += sizeof(T); }
template <class T> static void eeGet(int& a, T& v)       { EEPROM.get(a, v); a += sizeof(T); }

void saveState() {
  int a = 0;
  eePut(a, STATE_SIGNATURE);
  eePut(a, settings);
  for (int s = 0; s < SERIES_COUNT; s++) {
    eePut(a, series[s].head);
    eePut(a, series[s].fill);
    eePut(a, series[s].times);
    eePut(a, series[s].values);
  }
}

bool loadState() {
  int a = 0;
  uint32_t sig;
  eeGet(a, sig);
  if (sig != STATE_SIGNATURE) return false;
  eeGet(a, settings);
  for (int s = 0; s < SERIES_COUNT; s++) {
    eeGet(a, series[s].head);
    eeGet(a, series[s].fill);
    eeGet(a, series[s].times);
    eeGet(a, series[s].values);
  }
  return true;
}