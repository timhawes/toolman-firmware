#include "app_setup.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266httpUpdate.h>

bool setup_mode_changed = false;

void setup_mode_save_callback()
{
  setup_mode_changed = true;
}

SetupMode::SetupMode(const char *clientid, const char *setup_password)
{
  _clientid = clientid;
  _setup_password = setup_password;
  strncpy(server_host, "", sizeof(server_host));
  strncpy(server_port, "13259", sizeof(server_port));
  strncpy(server_password, "", sizeof(server_password));
}

void SetupMode::run()
{
  WiFiManager wifiManager;

  Serial.println("in setup mode now");

  if (SPIFFS.exists("config.json")) {
    File configFile = SPIFFS.open("config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        Serial.println("json parsed, loading into variables");
        strncpy(server_host, json["server_host"], sizeof(server_host));
        snprintf(server_port, sizeof(server_port), "%d", json["server_port"]);
        strncpy(server_port, json["server_port"], sizeof(server_port));
        strncpy(server_password, json["server_password"], sizeof(server_password));
        Serial.print("server_host loaded as ");
        Serial.println(server_host);
      }
    }
  }

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
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["ssid"] = WiFi.SSID();
      json["wpa_password"] = WiFi.psk();
      json["server_host"] = custom_host.getValue();
      json["server_port"] = atoi(custom_port.getValue());
      json["server_password"] = custom_password.getValue();

      Serial.println("attempting to write config.json");
      File configFile = SPIFFS.open("config.json", "w");
      if (configFile) {
        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        Serial.println("wrote config.json");
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
        Serial.println("failed to write config.json");
        delay(500);
        ESP.restart();
      }
    }
  }
  Serial.println("leaving setup mode, restart");
  delay(500);
  ESP.restart();
}
