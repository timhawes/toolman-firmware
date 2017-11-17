#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <Arduino.h>

struct config_t {
  bool changed;
  char ssid[64];
  char wpa_password[64];
  char server_host[64];
  uint16_t server_port;
  char server_password[32];
  char setup_password[32];
  char name[20];
  unsigned long idle_timeout; // ms
  unsigned long active_timeout; // ms
  unsigned long adc_interval; // ms
  unsigned long adc_multiplier;
  unsigned long active_threshold; // milliamps
  unsigned long card_present_timeout; // ms
  unsigned long net_keepalive_interval; // ms
  unsigned long net_retry_min_interval; // ms
  unsigned long net_retry_max_interval; // ms
  unsigned long long_press_time; // ms
  unsigned long report_status_interval; // ms
  unsigned long token_query_timeout; // ms
  bool dev;
};

struct queued_message_t {
  unsigned long seq;
  unsigned long last_sent;
  long interval;
  unsigned int attempts;
  unsigned int len;
  uint8_t *data;
};

#endif
