#ifndef APP_TCP_H
#define APP_TCP_H

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#include <ByteBuffer.h>

#include "app_network.h"
#include "app_types.h"

class TcpNet : public Network
{
private:
  config_t *_config;
  bool wifi_state_connected;
  bool client_connected;
  AsyncClient *client;
  unsigned long last_send;
  unsigned long last_connect;
  unsigned long last_receive;
  const unsigned int connect_interval = 3000;
  unsigned long seq = 0;
  //unsigned long ping_interval = 0;
  //unsigned long last_ping = 0;
  bool buffer_incoming_data(uint8_t *message, uint16_t len);
  void process_buffered_data();
  void run_client();
  uint8_t outgoing_packet[1024];
  unsigned int outgoing_packet_len = 0;
  ByteBuffer *buffer;
  void abort();
public:
  TcpNet();
  bool send(const uint8_t *data, int len);
  bool send_json(JsonObject &data);
  void begin(config_t &config);
  void loop();
  void wifi_connected();
  void wifi_disconnected();
  bool connected();
  network_state_callback_t state_callback = NULL;
  received_message_callback_t received_message_callback = NULL;
  received_json_callback_t received_json_callback = NULL;
};

#endif
