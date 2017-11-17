#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class Network
{
private:
  const char *_ssid;
  const char *_wpa_password;
  const char *_host;
  uint16_t _port;
  const char *_clientid;
  const char *_password;
  bool wifi_connected;
  bool client_connected;
  WiFiUDP *client;
  unsigned long last_connect;
  unsigned long last_send;
  unsigned long last_receive;
  const unsigned int connect_interval = 3000;
  char send_buffer[100];
  uint8_t send_len = 0;
  void udp_receive();
  void udp_send();
  void incoming_handler(uint8_t message[], uint16_t len);
public:
  Network(const char *ssid, const char *wpa_password, const char *host, uint16_t port, const char *clientid, const char *password);
  void send(String data);
  void begin();
  void loop();
};

#endif
