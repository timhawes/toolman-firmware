#ifndef APP_SETUP_H
#define APP_SETUP_H

#include <Arduino.h>
#include <WiFiManager.h>
#include "app_display.h"

class SetupMode
{
private:
  const char *_clientid;
  const char *_setup_password;
  char server_host[100];
  char server_port[6];
  char server_password[50];
  char firmware_url[255];
public:
  SetupMode(const char *clientid, const char *setup_password);
  void run();
};

#endif
