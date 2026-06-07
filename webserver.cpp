#include "monitor.h"
#include <WiFiS3.h>

static WiFiServer server(80);

static void setLimit(String key, float v) {
  if (key == "tempcool")        settings.tempCool = v;
  else if (key == "tempwarm")   settings.tempWarm = v;
  else if (key == "humiddry")   settings.humidDry = v;
  else if (key == "humidhumid") settings.humidHumid = v;
  else if (key == "co2low")     settings.co2Low = (int)v;
  else if (key == "co2high")    settings.co2High = (int)v;
  else if (key == "night")      settings.nightLevel = (int)v;
  else if (key == "buzzertemp")    settings.buzzer[TEMP]    = v != 0;
  else if (key == "buzzerhumid")   settings.buzzer[HUMID]   = v != 0;
  else if (key == "buzzerco2")     settings.buzzer[CO2]     = v != 0;
  else if (key == "telegramtemp")  settings.telegram[TEMP]  = v != 0;
  else if (key == "telegramhumid") settings.telegram[HUMID] = v != 0;
  else if (key == "telegramco2")   settings.telegram[CO2]   = v != 0;
}

static long queryNum(String &req, const char* key, long fallback) {
  int i = req.indexOf(key);
  if (i < 0) return fallback;
  i += strlen(key);
  int j = i;
  while (j < (int)req.length() && isDigit(req[j])) j++;
  if (j == i) return fallback;
  return req.substring(i, j).toInt();
}

static void sendOk(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("ok");
  client.stop();
}

static void sendApi(WiFiClient &client, String &req) {
  unsigned long now  = clockNow();
  unsigned long to   = (unsigned long)queryNum(req, "to=", now);
  unsigned long from = (unsigned long)queryNum(req, "from=", now > 86400UL ? now - 86400UL : 0);
  if (to <= from) { to = now; from = now > 86400UL ? now - 86400UL : 0; }

  Series &sr = series[pickSeries(from)];

  int stored = 0, capacity = 0;
  unsigned long since = 0;
  for (int s = 0; s < SERIES_COUNT; s++) {
    stored += series[s].fill;
    capacity += series[s].capacity;
    if (series[s].fill > 0) {
      int o = (series[s].head - series[s].fill + series[s].capacity) % series[s].capacity;
      if (since == 0 || series[s].times[o] < since) since = series[s].times[o];
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();

  String reason = notifyReason();
  client.print("{\"time\":");      client.print(now);
  client.print(",\"notify\":");    client.print(reason.length() ? "true" : "false");
  client.print(",\"notifyReason\":\""); client.print(reason); client.print("\"");
  client.print(",\"stored\":");   client.print(stored);
  client.print(",\"capacity\":"); client.print(capacity);
  client.print(",\"since\":");    client.print(since);
  client.print(",\"step\":");     client.print(sr.intervalMs / 1000);

  client.print(",\"limits\":{");
  client.print("\"tempCool\":");    client.print(settings.tempCool, 1);
  client.print(",\"tempWarm\":");   client.print(settings.tempWarm, 1);
  client.print(",\"humidDry\":");   client.print(settings.humidDry, 0);
  client.print(",\"humidHumid\":"); client.print(settings.humidHumid, 0);
  client.print(",\"co2Low\":");   client.print(settings.co2Low);
  client.print(",\"co2High\":");  client.print(settings.co2High);
  client.print(",\"night\":");      client.print(settings.nightLevel);
  client.print(",\"buzzerTemp\":");    client.print(settings.buzzer[TEMP]    ? 1 : 0);
  client.print(",\"buzzerHumid\":");   client.print(settings.buzzer[HUMID]   ? 1 : 0);
  client.print(",\"buzzerCO2\":");     client.print(settings.buzzer[CO2]     ? 1 : 0);
  client.print(",\"telegramTemp\":");  client.print(settings.telegram[TEMP]  ? 1 : 0);
  client.print(",\"telegramHumid\":"); client.print(settings.telegram[HUMID] ? 1 : 0);
  client.print(",\"telegramCO2\":");   client.print(settings.telegram[CO2]   ? 1 : 0);
  client.print("}");

  client.print(",\"times\":[");
  bool first = true;
  for (int i = 0; i < sr.fill; i++) {
    int p = (sr.head - sr.fill + i + sr.capacity) % sr.capacity;
    uint32_t t = sr.times[p];
    if (t >= from && t <= to) { if (!first) client.print(","); client.print(t); first = false; }
  }
  client.print("]");

  client.print(",\"readings\":[");
  for (int m = 0; m < READING_COUNT; m++) {
    Reading &r = readings[m];
    if (m > 0) client.print(",");
    client.print("{\"key\":\"");    client.print(r.key);    client.print("\"");
    client.print(",\"unit\":\"");   client.print(r.unit);   client.print("\"");
    client.print(",\"decimals\":"); client.print(r.decimals);
    client.print(",\"value\":");    client.print(r.value, r.decimals);
    client.print(",\"status\":\""); client.print(rangeStatus(m)); client.print("\"");
    client.print(",\"history\":[");
    bool f = true;
    for (int i = 0; i < sr.fill; i++) {
      int p = (sr.head - sr.fill + i + sr.capacity) % sr.capacity;
      if (sr.times[p] >= from && sr.times[p] <= to) { if (!f) client.print(","); client.print(sr.values[p][m], r.decimals); f = false; }
    }
    client.print("]}");
  }
  client.print("]}");
  client.println();
  client.stop();
}

static void sendDashboard(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();
  client.println(dashboardPage());
  client.stop();
}

void webBegin() {
  server.begin();
}

void handleWeb() {
  WiFiClient client = server.available();
  if (!client) return;

  String req = "";
  unsigned long start = millis();
  while (client.connected() && millis() - start < 1000) {
    if (client.available()) {
      char c = client.read();
      req += c;
      if (c == '\n' && req.endsWith("\r\n\r\n")) break;
    }
  }

  if (req.indexOf("GET /calibrate") >= 0) {
    calibrate();
    sendOk(client);
    return;
  }

  if (req.indexOf("GET /set/") >= 0) {
    int p = req.indexOf("/set/") + 5;
    int slash = req.indexOf("/", p);
    String key = req.substring(p, slash);
    float value = req.substring(slash + 1, req.indexOf(" ", slash + 1)).toFloat();
    setLimit(key, value);
    saveState();
    sendOk(client);
    return;
  }

  if (req.indexOf("GET /api") >= 0) { sendApi(client, req); return; }

  sendDashboard(client);
}