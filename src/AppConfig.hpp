// SPDX-FileCopyrightText: 2019-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <Arduino.h>

class AppConfig {
 public:
  AppConfig();
  bool dev;
  bool events;
  bool nfc_read_counter;
  bool nfc_read_sig;
  bool server_tls_enabled;
  bool server_tls_verify;
  bool quiet;
  bool swap_buttons;
  bool reboot_enabled;
  char name[21];
  char server_host[64];
  char server_password[64];
  char ssid[33];
  char wpa_password[64];
  int active_threshold;
  int adc_multiplier;
  int network_conn_stable_time;
  int network_reconnect_max_time;
  int network_watchdog_time;
  int nfc_read_data;
  int nfc_check_interval = 10000;
  int nfc_reset_interval = 1000;
  int nfc_5s_limit = 30;
  int nfc_1m_limit = 60;
  int server_port;
  long adc_interval;
  long idle_timeout;
  long token_query_timeout;
  uint8_t server_fingerprint1[21];
  uint8_t server_fingerprint2[21];
  void LoadDefaults();
  bool LoadWifiJson(const char *filename = "wifi.json");
  bool LoadNetJson(const char *filename = "net.json");
  bool LoadAppJson(const char *filename = "app.json");
  void LoadOverrides();
};

#endif
