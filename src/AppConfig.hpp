// SPDX-FileCopyrightText: 2019-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <Arduino.h>
#include "config.h"

class AppConfig {
 public:
  AppConfig();
  // wifi
  char ssid[33];
  char wpa_password[64];
  // net
  bool server_tls_enabled;
  bool server_tls_verify;
  char server_host[64];
  char server_password[64];
  int network_conn_stable_time;
  int network_reconnect_max_time;
  int network_watchdog_time;
  int server_port;
  uint8_t server_fingerprint1[21];
  uint8_t server_fingerprint2[21];
  // app
  bool dev;
  bool events;
  bool nfc_read_counter;
  bool nfc_read_sig;
  bool quiet;
  bool reboot_enabled;
  bool swap_buttons;
  char name[21];
  int active_threshold;
  int adc_multiplier;
  int nfc_1m_limit;
  int nfc_5s_limit;
  int nfc_check_interval;
  int nfc_read_data;
  int nfc_reset_interval;
  long adc_interval;
  long idle_timeout;
  long token_query_timeout;
  void LoadDefaults();
  bool LoadWifiJson(const char *filename = WIFI_JSON_FILENAME);
  bool LoadNetJson(const char *filename = NET_JSON_FILENAME);
  bool LoadAppJson(const char *filename = APP_JSON_FILENAME);
  void LoadOverrides();
};

#endif
