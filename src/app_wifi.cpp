#include "app_wifi.h"

WifiSupervisor::WifiSupervisor()
{
}

void WifiSupervisor::begin(ArduinoConfigDB &config)
{
  WiFi.mode(WIFI_STA);
  _config = &config;
  connected = false;
}

void WifiSupervisor::loop()
{
  static bool begin_done = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!connected) {
      Serial.print("wifi connected ");
      Serial.println(WiFi.localIP());
      connected = true;
      if (wifi_connected_callback) {
        wifi_connected_callback();
      }
    }
  } else {
    if (!begin_done) {
      WiFi.begin(_config->getConstChar("ssid"), _config->getConstChar("wpa_password"));
      begin_done = true;
    }
    if (connected) {
      Serial.println("wifi disconnected");
      connected = false;
      if (wifi_disconnected_callback) {
        wifi_disconnected_callback();
      }
    }
  }

}
