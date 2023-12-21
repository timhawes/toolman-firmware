// SPDX-FileCopyrightText: 2017-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <Arduino.h>
#include <DNSServer.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include "config.h"

class SetupMode
{
private:
  const char *_ssid;
  const char *_password;
#ifdef ESP8266
  ESP8266WebServer server;
#else
  WebServer server;
#endif
  void configRootHandler();
  void configUpdateHandler();
public:
  SetupMode(const char *ssid, const char *password);
  void run();
};

#endif
