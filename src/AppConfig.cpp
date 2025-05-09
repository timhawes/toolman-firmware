// SPDX-FileCopyrightText: 2019-2025 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AppConfig.hpp"
#include <ArduinoJson.h>
#include <FS.h>
#ifdef ESP8266
#include "app_util.h"
#else
#include "SPIFFS.h"
#endif

AppConfig::AppConfig() {
  LoadDefaults();
  LoadOverrides();
}

void AppConfig::LoadDefaults() {
  // wifi
  strlcpy(ssid, "", sizeof(ssid));
  strlcpy(wpa_password, "", sizeof(wpa_password));
  wifi_check_interval = 60000;
  // net
  strlcpy(name, "Unconfigured", sizeof(name));
  strlcpy(server_host, "", sizeof(server_host));
  strlcpy(server_password, "", sizeof(server_password));
  network_conn_stable_time = 30000;
  network_reconnect_max_time = 300000;
  network_watchdog_time = 3600000;
  server_port = 13260;
  server_tls_enabled = false;
  server_tls_verify = false;
  // app
  active_threshold = 500;
  adc_interval = 1000;
  ct_cal = 1.0;
  ct_ratio = 1.0;
  ct_resistor = 1.0;
  dev = false;
  events = true;
  idle_timeout = 600000;
  idle_warning_beep = 100;
  idle_warning_timeout = 60000;
  nfc_1m_limit = 60;
  nfc_5s_limit = 30;
  nfc_check_interval = 10000;
  nfc_read_counter = false;
  nfc_read_data = 0;
  nfc_read_sig = false;
  nfc_reset_interval = 1000;
  quiet = false;
  reboot_enabled = false;
  show_idle = false;
  swap_buttons = false;
  token_query_timeout = 1000;
}

void AppConfig::LoadOverrides() {

}

bool AppConfig::LoadWifiJson(const char *filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("AppConfig: wifi file not found");
    LoadOverrides();
    return false;
  }

  DynamicJsonDocument root(4096);
  DeserializationError err = deserializeJson(root, file);
  file.close();

  if (err) {
    Serial.print("AppConfig: failed to parse JSON: ");
    Serial.println(err.c_str());
    LoadOverrides();
    return false;
  }

  root["ssid"].as<String>().toCharArray(ssid, sizeof(ssid));
  root["password"].as<String>().toCharArray(wpa_password, sizeof(wpa_password));
  wifi_check_interval = root["wifi_check_interval"] | 60000;

  LoadOverrides();

  Serial.println("AppConfig: wifi loaded");
  return true;
}

bool AppConfig::LoadNetJson(const char *filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("AppConfig: net file not found");
    LoadOverrides();
    return false;
  }

  DynamicJsonDocument root(4096);
  DeserializationError err = deserializeJson(root, file);
  file.close();

  if (err) {
    Serial.print("AppConfig: failed to parse JSON: ");
    Serial.println(err.c_str());
    LoadOverrides();
    return false;
  }

  root["host"].as<String>().toCharArray(server_host, sizeof(server_host));
  root["password"].as<String>().toCharArray(server_password, sizeof(server_password));
#ifdef ESP32
  root["tls_sha256_fingerprint1"].as<String>().toCharArray(server_sha256_fingerprint1, sizeof(server_sha256_fingerprint1));
  root["tls_sha256_fingerprint2"].as<String>().toCharArray(server_sha256_fingerprint2, sizeof(server_sha256_fingerprint2));
#endif

  network_conn_stable_time = root["conn_stable_time"] | 30000;
  network_reconnect_max_time = root["reconnect_max_time"] | 300000;
  network_watchdog_time = root["watchdog_time"] | 3600000;
  server_port = root["port"] | 13260;
  server_tls_enabled = root["tls"] | false;
  server_tls_verify = root["tls_verify"] | false;

#ifdef ESP8266
  memset(server_fingerprint1, 0, sizeof(server_fingerprint1));
  memset(server_fingerprint2, 0, sizeof(server_fingerprint2));
  decode_hex(root["tls_fingerprint1"].as<String>().c_str(),
             server_fingerprint1, sizeof(server_fingerprint1));
  decode_hex(root["tls_fingerprint2"].as<String>().c_str(),
             server_fingerprint2, sizeof(server_fingerprint2));
#endif

  LoadOverrides();

  Serial.println("AppConfig: net loaded");
  return true;
}

bool AppConfig::LoadAppJson(const char *filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("AppConfig: app file not found");
    LoadOverrides();
    return false;
  }

  DynamicJsonDocument root(4096);
  DeserializationError err = deserializeJson(root, file);
  file.close();

  if (err) {
    Serial.print("AppConfig: failed to parse JSON: ");
    Serial.println(err.c_str());
    LoadOverrides();
    return false;
  }

  root["name"].as<String>().toCharArray(name, sizeof(name));

  active_threshold = root["active_threshold"] | 500;
  adc_interval = root["adc_interval"] | 1000;
  ct_cal = root["ct_cal"] | 1.0;
  ct_ratio = root["ct_ratio"] | 2000.0;
  ct_resistor = root["ct_resistor"] | 33.0;
  dev = root["dev"] | false;
  events = root["events"] | true;
  idle_timeout = root["idle_timeout"] | 600000;
  idle_warning_beep = root["idle_warning_beep"] | 100;
  idle_warning_timeout = root["idle_warning_timeout"] | 60000;
  nfc_1m_limit = root["nfc_1m_limit"] | 60;
  nfc_5s_limit = root["nfc_5s_limit"] | 30;
  nfc_check_interval = root["nfc_check_interval"] | 10000;
  nfc_read_counter = root["nfc_read_counter"] | false;
  nfc_read_data = root["nfc_read_data"] | 0;
  nfc_read_sig = root["nfc_read_sig"] | false;
  nfc_reset_interval = root["nfc_reset_interval"] | 1000;
  quiet = root["quiet"] | false;
  reboot_enabled = root["reboot_enabled"] | false;
  show_idle = root["show_idle"] | false;
  swap_buttons = root["swap_buttons"] | false;
  token_query_timeout = root["token_query_timeout"] | 1000;

  LoadOverrides();

  Serial.println("AppConfig: app loaded");
  return true;
}
