#include "monitor.h"
#include <WiFiS3.h>

static long lastUpdateId = 0;

void wifiConnect() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  showMessage("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "WiFi failed");
}

String ipString() {
  IPAddress ip = WiFi.localIP();
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

static String httpsGet(const char* host, String path, bool capture) {
  if (WiFi.status() != WL_CONNECTED) return "";
  WiFiSSLClient client;
  if (!client.connect(host, 443)) return "";
  client.println("GET " + path + " HTTP/1.1");
  client.println("Host: " + String(host));
  client.println("Connection: close");
  client.println();
  unsigned long timeout = millis();
  while (!client.available() && millis() - timeout < 3000) delay(10);
  String out = "";
  while (client.available()) {
    char c = client.read();
    if (capture) out += c;
  }
  client.stop();
  return out;
}

void sendTelegram(String message) {
  message.replace(" ", "%20");
  message.replace("\n", "%0A");
  httpsGet("api.telegram.org",
           "/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + message,
           false);
}

static String statusText() {
  String s = "Status\n";
  for (int i = 0; i < READING_COUNT; i++)
    s += String(readings[i].label) + ": " + formatValue(i) + " (" + rangeStatus(i) + ")\n";
  String r = notifyReason();
  s += r.length() ? "Notifications: " + r : "No active notifications";
  return s;
}

static String helpText() {
  return "Commands\n/status - readings and air quality\n/calibrate - reset CO2 baseline in fresh air\nLimits are set on the dashboard.";
}

static void handleCommand(String command) {
  command.trim();
  if (!command.startsWith("/")) return;
  command.toLowerCase();
  int at = command.indexOf('@');
  if (at > 0) command = command.substring(0, at);
  if (command == "/status") sendTelegram(statusText());
  else if (command == "/calibrate") { calibrate(); sendTelegram("Calibrated. R0 = " + String(settings.r0, 2)); }
  else if (command == "/help") sendTelegram(helpText());
  else sendTelegram("Unknown command. Send /help.");
}

void checkTelegram() {
  String response = httpsGet("api.telegram.org",
                             "/bot" + String(BOT_TOKEN) + "/getUpdates?offset=" + String(lastUpdateId + 1) + "&limit=1&timeout=0",
                             true);

  int idx = response.indexOf("\"update_id\":");
  if (idx == -1) return;
  long updateId = response.substring(idx + 12, response.indexOf(",", idx)).toInt();
  if (updateId <= lastUpdateId) return;
  lastUpdateId = updateId;

  if (response.indexOf("\"id\":" + String(CHAT_ID)) == -1) return;

  int textIdx = response.indexOf("\"text\":\"");
  if (textIdx == -1) return;
  String command = response.substring(textIdx + 8, response.indexOf("\"", textIdx + 8));
  handleCommand(command);
}

void logToSheets() {
  httpsGet(SHEETS_HOST,
           String(SHEETS_PATH) +
             "?temp="  + String(readings[TEMP].value, 1) +
             "&humid=" + String(readings[HUMID].value, 1) +
             "&co2="   + String(readings[CO2].value, 0) +
             "&light=" + String(readings[LIGHT].value, 0),
           false);
}
