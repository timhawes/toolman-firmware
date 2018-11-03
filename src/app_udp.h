#ifndef APP_UDP_H
#define APP_UDP_H

extern "C"{
#include "lwip/ip.h"
#include "lwip/dns.h"
};

#include <ArduinoJson.h>
#include <TimsReliableDatagram.h>

#include "app_network.h"
#include "app_types.h"

class UdpNet : public Network
{
private:
  config_t *_config;
  bool wifi_state;
  bool client_state;
  unsigned long last_send;
  unsigned long last_connect;
  unsigned long last_receive;
  const unsigned int connect_interval = 3000;
  unsigned long seq = 0;
  void abort();
  bool send_hello();
  void receive(const uint8_t *data, unsigned int len);
public:
  UdpNet();
  MyUdp myudp;
  bool send(const uint8_t *data, int len);
  bool send_json(JsonObject &data);
  void begin(config_t &config);
  void loop();
  void wifi_connected();
  void wifi_disconnected();
  bool connected();
  void stop();
  void flush();
  unsigned long get_retry_count();
  received_json_callback_t received_json_callback = NULL;
  unsigned long send_failip_count = 0;
  unsigned long send_failbegin_count = 0;
  unsigned long send_failwrite_count = 0;
  unsigned long send_failend_count = 0;
};

#endif
