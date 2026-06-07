#pragma once
#include <Arduino.h>
#include "config.h"

enum { TEMP, HUMID, CO2, LIGHT, READING_COUNT };
enum { HOUR, DAY, WEEK, SERIES_COUNT };

const int MAX_SLOTS = 96;

struct Reading {
  const char* key;
  const char* label;
  const char* unit;
  byte decimals;
  float value;
};

struct Series {
  int capacity;
  unsigned long intervalMs;
  int head;
  int fill;
  uint32_t times[MAX_SLOTS];
  float values[MAX_SLOTS][READING_COUNT];
  unsigned long lastSampleMs;
};

struct Settings {
  float tempCool, tempWarm;
  float humidDry, humidHumid;
  int co2Low, co2High;
  int nightLevel;
  float r0;
  bool buzzer[READING_COUNT];
  bool telegram[READING_COUNT];
};

extern Reading readings[READING_COUNT];
extern Series series[SERIES_COUNT];
extern Settings settings;
extern unsigned long bootEpoch;

void sensorsBegin();
void defineReading(int i, const char* key, const char* label, const char* unit, byte decimals);
void readSensors();
void calibrate();

void alertsBegin();
const char* rangeStatus(int i);
String formatValue(int i);
void setLED(int r, int g, int b);
String notifyReason();
void updateAlerts();

void displayBegin();
void drawScreen();
void showMessage(const char* text);

unsigned long clockNow();
void syncClock();
String clockHHMM();
void recordSample(int s);
int pickSeries(unsigned long from);
void saveState();
bool loadState();

void wifiConnect();
String ipString();
void sendTelegram(String message);
void checkTelegram();
void logToSheets();

void webBegin();
void handleWeb();
const char* dashboardPage();