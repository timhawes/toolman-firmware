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
  strlcpy(server_host, "ss-tool-controller.hacklab", sizeof(server_host));
  strlcpy(server_password, "password", sizeof(server_password));
  strlcpy(ssid, "Hacklab", sizeof(ssid));
  strlcpy(wpa_password, "", sizeof(wpa_password));
  active_threshold = 500;
  adc_interval = 1000;
  adc_multiplier = 1;
  dev = false;
  idle_timeout = 600000;
  nfc_read_counter = false;
  nfc_read_data = 0;
  nfc_read_sig = false;
  server_port = 13260;
  server_tls_enabled = false;
  server_tls_verify = false;
  swap_buttons = false;
  token_query_timeout = 1000;
}

void AppConfig::LoadOverrides() {

}

bool AppConfig::LoadJson(const char *filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("AppConfig: file not found");
    return false;
  }

  DynamicJsonBuffer jb;
  JsonObject &root = jb.parseObject(file);
  file.close();

  if (!root.success()) {
    Serial.println("AppConfig: failed to parse JSON");
    return false;
  }

  root["name"].as<String>().toCharArray(name, sizeof(name));
  root["server_host"].as<String>().toCharArray(server_host, sizeof(server_host));
  root["server_password"].as<String>().toCharArray(server_password, sizeof(server_password));
  root["ssid"].as<String>().toCharArray(ssid, sizeof(ssid));
  root["wpa_password"].as<String>().toCharArray(wpa_password, sizeof(wpa_password));

  active_threshold = root["active_threshold"];
  adc_interval = root["adc_interval"];
  adc_multiplier = root["adc_multiplier"];
  dev = root["dev"];
  idle_timeout = root["idle_timeout"];
  nfc_read_counter = root["nfc_read_counter"];
  nfc_read_data = root["nfc_read_data"];
  nfc_read_sig = root["nfc_read_sig"];
  server_port = root["server_port"];
  server_tls_enabled = root["server_tls_enabled"];
  server_tls_verify = root["server_tls_verify"];
  swap_buttons = root["swap_buttons"];
  token_query_timeout = root["token_query_timeout"];

  memset(server_fingerprint1, 0, sizeof(server_fingerprint1));
  memset(server_fingerprint2, 0, sizeof(server_fingerprint2));
  decode_hex(root["server_fingerprint1"].as<String>().c_str(),
             server_fingerprint1, sizeof(server_fingerprint1));
  decode_hex(root["server_fingerprint2"].as<String>().c_str(),
             server_fingerprint2, sizeof(server_fingerprint2));

  LoadOverrides();

  Serial.println("AppConfig: loaded");
  return true;
}
