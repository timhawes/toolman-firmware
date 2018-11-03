#ifndef TIMSRELIABLEDATAGRAM_H
#define TIMSRELIABLEDATAGRAM_H

extern "C"{
#include "lwip/ip.h"
#include "lwip/dns.h"
};

#include <functional>
#include <cbuf.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

typedef void (*myudp_callback_t)(const uint8_t *data, unsigned int len);
typedef std::function<void(const uint8_t*, unsigned int len)> fancy_callback_t;

class MyUdp
{
private:
  WiFiUDP client;
  IPAddress ipaddress;
  cbuf *buf;
  ip_addr_t lookup_addr;
  char hostname[127];
  unsigned int port;
  unsigned int local_port;
  unsigned long last_lookup;
  long lookup_interval = 60000;
  long lookup_retry = 5000;
  long min_retry = 100;
  long max_retry = 10000;
  long retry_interval = min_retry;
  int dup_tolerance = 100;
  void start_lookup();
  unsigned int buf_next_size();
  bool debug_mode = false;

  //char buffer[1000];
  uint16_t buffer_len = 0;
  uint16_t buffer_seq = 0;
  //uint16_t acked_seq = -1;
  uint16_t last_received_seq;
  bool first_received = false;
  //unsigned long buffer_time;
  unsigned long last_send;
  //int16_t rx_seq = -1;
  //int16_t rx_ack = -1;
  unsigned long last_receive;
  unsigned int attempts = 0;
  unsigned long retry_count = 0; // running counter for statistics reporting

public:
  MyUdp();
  void begin(const char *hostname, unsigned int port);
  void set_hostname(const char *hostname);
  void set_port(unsigned int port);
  void loop();
  bool send(const uint8_t *data, int len);
  void flush();
  void debug(bool enabled);
  bool status();
  unsigned int get_attempts();
  unsigned long get_retry_count();
  myudp_callback_t read_callback;
  fancy_callback_t fancy_callback;
};

#endif
