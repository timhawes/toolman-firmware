// SPDX-FileCopyrightText: 2019-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AppConfig.hpp"
#include <ArduinoJson.h>
#include <FS.h>
#include "app_util.h"

AppConfig::AppConfig() {
  LoadDefaults();
  LoadOverrides();
}

void AppConfig::LoadDefaults() {
  strlcpy(name, "Unconfigured", sizeof(name));
  strlcpy(server_host, "", sizeof(server_host));
  strlcpy(server_password, "", sizeof(server_password));
  strlcpy(ssid, "", sizeof(ssid));
  strlcpy(wpa_password, "", sizeof(wpa_password));
  active_threshold = 500;
  adc_interval = 1000;
  adc_multiplier = 1;
  dev = false;
  events = false;
  idle_timeout = 600000;
  network_conn_stable_time = 30000;
  network_reconnect_max_time = 300000;
  network_watchdog_time = 3600000;
  nfc_read_counter = false;
  nfc_read_data = 0;
  nfc_read_sig = false;
  nfc_check_interval = 10000;
  nfc_reset_interval = 1000;
  nfc_5s_limit = 30;
  nfc_1m_limit = 60;
  reboot_enabled = false;
  server_port = 13260;
  server_tls_enabled = false;
  server_tls_verify = false;
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

  network_conn_stable_time = root["conn_stable_time"] | 30000;
  network_reconnect_max_time = root["reconnect_max_time"] | 300000;
  network_watchdog_time = root["watchdog_time"] | 3600000;
  server_port = root["port"];
  server_tls_enabled = root["tls"];
  server_tls_verify = root["tls_verify"];

  memset(server_fingerprint1, 0, sizeof(server_fingerprint1));
  memset(server_fingerprint2, 0, sizeof(server_fingerprint2));
  decode_hex(root["tls_fingerprint1"].as<String>().c_str(),
             server_fingerprint1, sizeof(server_fingerprint1));
  decode_hex(root["tls_fingerprint2"].as<String>().c_str(),
             server_fingerprint2, sizeof(server_fingerprint2));

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

  active_threshold = root["active_threshold"];
  adc_interval = root["adc_interval"];
  adc_multiplier = root["adc_multiplier"];
  dev = root["dev"];
  events = root["events"] | false;
  idle_timeout = root["idle_timeout"];
  nfc_read_counter = root["nfc_read_counter"] | false;
  nfc_read_data = root["nfc_read_data"] | 0;
  nfc_read_sig = root["nfc_read_sig"] | false;
  nfc_check_interval = root["nfc_check_interval"] | 10000;
  nfc_reset_interval = root["nfc_reset_interval"] | 1000;
  nfc_5s_limit = root["nfc_5s_limit"] | 30;
  nfc_1m_limit = root["nfc_1m_limit"] | 60;
  reboot_enabled = root["reboot_enabled"];
  swap_buttons = root["swap_buttons"];
  token_query_timeout = root["token_query_timeout"];

  LoadOverrides();

  Serial.println("AppConfig: app loaded");
  return true;
}
