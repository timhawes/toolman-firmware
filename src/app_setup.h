#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <Arduino.h>
#include <ArduinoConfigDB.hpp>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include "app_display.h"

class SetupMode
{
private:
  const char *_clientid;
  const char *_setup_password;
  ESP8266WebServer server;
  ArduinoConfigDB *_config;
  std::map<String, String> mymap;
  void configRootHandler();
  void configUpdateHandler();
public:
  SetupMode(const char *clientid, const char *setup_password, ArduinoConfigDB *config);
  void run();
};

#endif
