#ifndef APP_WIFI_H
#define APP_WIFI_H

#include <ESP8266WiFi.h>

#include "app_types.h"
#include "ArduinoConfigDB.hpp"

typedef void (*wifi_connected_callback_t)();
typedef void (*wifi_disconnected_callback_t)();

class WifiSupervisor
{
private:
  ArduinoConfigDB *_config;
  bool connected;
public:
  WifiSupervisor();
  void begin(ArduinoConfigDB &config);
  void loop();
  wifi_connected_callback_t wifi_connected_callback = NULL;
  wifi_disconnected_callback_t wifi_disconnected_callback = NULL;
};

#endif
