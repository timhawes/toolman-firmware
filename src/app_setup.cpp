// SPDX-FileCopyrightText: 2017-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_setup.h"
#include <FS.h>
#include <ArduinoJson.h>

static const char html[] PROGMEM =
    "<form method='POST' action='/update' />\n"
    "<table>\n"
    "<tr><th>SSID</th><td><input type='text' name='ssid' /></td></tr>\n"
    "<tr><th>WPA Password</th><td><input type='text' name='wpa_password' /></td></tr>\n"
    "<tr><th>Server Host</th><td><input type='text' name='server_host' /></td></tr>\n"
    "<tr><th>Server Port</th><td><input type='text' name='server_port' value='13261' /></td></tr>\n"
    "<tr><th>Server TLS</th><td><input type='checkbox' name='server_tls_enabled' value='1' checked /></td></tr>\n"
    "<tr><th>Server TLS Fingerprint</th><td><input type='text' name='server_tls_fingerprint' /></td></tr>\n"
    "<tr><th>Server Password</th><td><input type='text' name='server_password' /></td></tr>\n"
    "</table>\n"
    "<input type='submit' value='Save and Restart' />"
    "</form>\n";

SetupMode::SetupMode(const char *ssid, const char *password) {
  _ssid = ssid;
  _password = password;
}

void SetupMode::configRootHandler() {
  String output = FPSTR(html);
  server.send(200, "text/html", output);
}

void SetupMode::configUpdateHandler() {
  DynamicJsonDocument root(4096);
  File file;

  for (int i=0; i<server.args(); i++) {
    if (server.argName(i) == "ssid") root["ssid"] = server.arg(i);
    if (server.argName(i) == "wpa_password") root["password"] = server.arg(i);
  }
  file = SPIFFS.open(WIFI_JSON_FILENAME, "w");
  serializeJson(root, file);
  file.close();

  root.clear();

  for (int i=0; i<server.args(); i++) {
    if (server.argName(i) == "server_host") root["host"] = server.arg(i);
    if (server.argName(i) == "server_port") root["port"] = server.arg(i).toInt();
    if (server.argName(i) == "server_tls_enabled") root["tls"] = (bool)server.arg(i).toInt();
    if (server.argName(i) == "server_tls_fingerprint" && !server.arg(i).isEmpty()) {
      root["tls_fingerprint1"] = server.arg(i);
      root["tls_verify"] = true;
    }
    if (server.argName(i) == "server_password") root["password"] = server.arg(i);
  }
  file = SPIFFS.open(NET_JSON_FILENAME, "w");
  serializeJson(root, file);
  file.close();

  server.sendHeader("Location", "/");
  server.send(301);
  delay(500);
  ESP.restart();
}

void SetupMode::run() {
  Serial.println("in setup mode now");

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(_ssid, _password);

  delay(100);
  IPAddress ip = WiFi.softAPIP();

  // display access details
  Serial.print("WiFi AP: SSID=");
  Serial.print(_ssid);
  Serial.print(" Password=");
  Serial.print(_password);
  Serial.print(" URL=http://");
  Serial.print(ip);
  Serial.println("/");

  server.on("/", std::bind(&SetupMode::configRootHandler, this));
  server.on("/update", std::bind(&SetupMode::configUpdateHandler, this));
  server.begin(80);

  while (1) {
    server.handleClient();
  }

  Serial.println("leaving setup mode, restart");
  delay(500);
  ESP.restart();
}
