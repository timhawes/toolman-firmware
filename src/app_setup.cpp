#include "app_setup.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266httpUpdate.h>

bool setup_mode_changed = false;

void setup_mode_save_callback()
{
  setup_mode_changed = true;
}

SetupMode::SetupMode(const char *clientid, const char *setup_password, ArduinoConfigDB *config)
{
  _clientid = clientid;
  _setup_password = setup_password;
  _config = config;
  strncpy(server_host, "", sizeof(server_host));
  strncpy(server_port, "13259", sizeof(server_port));
  strncpy(server_password, "", sizeof(server_password));
}

void SetupMode::run()
{
  WiFiManager wifiManager;

  Serial.println("in setup mode now");

  strncpy(server_host, _config->getConstChar("server_host"), sizeof(server_host));
  strncpy(server_port, _config->getConstChar("server_port"), sizeof(server_port));
  strncpy(server_password, _config->getConstChar("server_password"), sizeof(server_password));

  WiFiManagerParameter custom_host("host", "host", server_host, sizeof(server_host));
  WiFiManagerParameter custom_port("port", "port", server_port, sizeof(server_port));
  WiFiManagerParameter custom_password("password", "password", server_password, sizeof(server_password));
  WiFiManagerParameter custom_firmware("firmware_url", "firmware url", "", sizeof(firmware_url));
  wifiManager.addParameter(&custom_host);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_firmware);
  wifiManager.setSaveConfigCallback(setup_mode_save_callback);

  //WiFi.disconnect();
  //delay(100);
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  if (wifiManager.autoConnect(_clientid, _setup_password)) {
    Serial.println("connected!");
  } else {
    Serial.println("not connected :-(");
  }
  while (1) {
    delay(100);
    if (setup_mode_changed) {
      Serial.println("change detected");
      _config->set("ssid", WiFi.SSID());
      _config->set("wpa_password", WiFi.psk());
      _config->set("server_host", custom_host.getValue());
      _config->set("server_port", custom_port.getValue());
      _config->set("server_password", custom_password.getValue());
      _config->save();
      if (!_config->hasChanged()) {
        if (strlen(custom_firmware.getValue()) > 0) {
          Serial.print("installing new firmware from ");
          Serial.println(custom_firmware.getValue());
          t_httpUpdate_return ret = ESPhttpUpdate.update(custom_firmware.getValue());
          switch(ret) {
            case HTTP_UPDATE_FAILED:
              Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
              break;
            case HTTP_UPDATE_NO_UPDATES:
              Serial.println("HTTP_UPDATE_NO_UPDATES");
              break;
            case HTTP_UPDATE_OK:
              Serial.println("HTTP_UPDATE_OK");
              break;
          }
          strncpy(firmware_url, "", sizeof(firmware_url));
        }
        Serial.println("restarting");
        delay(500);
        ESP.restart();
      } else {
        Serial.println("failed to save config");
        delay(500);
        ESP.restart();
      }
    }
  }
  Serial.println("leaving setup mode, restart");
  delay(500);
  ESP.restart();
}
