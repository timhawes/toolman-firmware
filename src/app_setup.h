// SPDX-FileCopyrightText: 2017-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>

class SetupMode
{
private:
  const char *_ssid;
  const char *_password;
  WebServer server;
  void configRootHandler();
  void configUpdateHandler();
public:
  SetupMode(const char *ssid, const char *password);
  void run();
};

#endif
