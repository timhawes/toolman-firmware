#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <Arduino.h>

class AppConfig {
 public:
  AppConfig();
  bool dev;
  bool nfc_read_counter;
  bool nfc_read_sig;
  bool server_tls_enabled;
  bool server_tls_verify;
  bool swap_buttons;
  bool reboot_enabled;
  char name[21];
  char server_host[64];
  char server_password[64];
  char ssid[33];
  char wpa_password[64];
  int active_threshold;
  int adc_multiplier;
  int net_watchdog_timeout;
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
  bool LoadJson(const char *filename = "config.json");
  void LoadOverrides();
};

#endif
