#include "monitor.h"
#include <U8g2lib.h>
#include <Wire.h>

static U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

void displayBegin() {
  display.begin();
  display.setFont(u8g2_font_6x10_tf);
}

static String ipLine() {
  String ip = ipString();
  return ip == "0.0.0.0" ? "offline" : ip;
}

static String trim0(float v) {
  String s = String(v, 1);
  if (s.endsWith(".0")) s = s.substring(0, s.length() - 2);
  return s;
}

static String rangeNote(int i) {
  float v = readings[i].value, lo, hi;
  switch (i) {
    case TEMP:  lo = settings.tempCool; hi = settings.tempWarm; break;
    case HUMID: lo = settings.humidDry; hi = settings.humidHumid; break;
    default:    lo = settings.co2Low; hi = settings.co2High; break;
  }
  if (v < lo) return "<" + trim0(lo);
  if (v > hi) return ">" + trim0(hi);
  return trim0(lo) + "-" + trim0(hi);
}

static void drawHeader() {
  display.setFont(u8g2_font_6x10_tf);
  String ip = ipLine(), clk = clockHHMM();
  display.drawStr(0, 10, ip.c_str());
  display.drawStr(128 - display.getStrWidth(clk.c_str()), 10, clk.c_str());
  display.drawHLine(0, 12, 128);
}

static void drawReadings() {
  display.clearBuffer();
  drawHeader();

  char line[24];
  snprintf(line, sizeof(line), "Temp   %.1f C",   readings[TEMP].value);  display.drawStr(0, 25, line);
  snprintf(line, sizeof(line), "Humid  %.1f %%",  readings[HUMID].value); display.drawStr(0, 37, line);
  snprintf(line, sizeof(line), "CO2    %.0f PPM", readings[CO2].value);   display.drawStr(0, 49, line);
  snprintf(line, sizeof(line), "Light  %.0f",     readings[LIGHT].value); display.drawStr(0, 61, line);

  display.sendBuffer();
}

static void drawRanges() {
  display.clearBuffer();
  drawHeader();
  display.setFont(u8g2_font_5x8_tf);

  static const char* names[] = { "Temp", "Humid", "CO2", "Light" };
  int y = 22;
  for (int i = 0; i < READING_COUNT; i++) {
    String s = String(names[i]) + " " + rangeStatus(i);
    if (i != LIGHT) s += " (" + rangeNote(i) + ")";
    display.drawStr(0, y, s.c_str());
    y += 10;
  }

  display.sendBuffer();
}

void drawScreen() {
  unsigned long phase = (millis() / 1000) % 16;
  if (phase < 11) drawReadings();
  else            drawRanges();
}

void showMessage(const char* text) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(4, 32, text);
  display.sendBuffer();
}