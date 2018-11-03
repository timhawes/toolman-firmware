#include "app_udp.h"

UdpNet::UdpNet()
{

}

void UdpNet::begin(config_t &config)
{
  using namespace std::placeholders;

  _config = &config;
  myudp.debug(false);
  //myudp.begin(config.server_host, config.server_port);
  myudp.begin(config.server_host, 33333);
  myudp.fancy_callback = std::bind(&UdpNet::receive, this, _1, _2);
  //wifi_state = false;
  //client_state = false;
}

void UdpNet::receive(const uint8_t *data, unsigned int len)
{
  if (received_json_callback) {
    received_json_callback((char*)data);
  }
}

void UdpNet::wifi_connected()
{
  //wifi_state = true;
  //client.begin(random(1024, 65534));
}

void UdpNet::wifi_disconnected()
{
  //wifi_state = false;
  //client_state = false;
}

void UdpNet::flush()
{
  myudp.flush();
}

bool UdpNet::send(const uint8_t *data, int len)
{
  return myudp.send(data, len);
}

bool UdpNet::send_json(JsonObject &data)
{
  char buf[data.measureLength()+1];
  data.printTo(buf, sizeof(buf));
  return send((const uint8_t*)buf, strlen(buf));
}

bool UdpNet::connected()
{
  return wifi_state && client_state;
}

void UdpNet::loop()
{
  static bool display_state = false;
  bool new_state = false;

  myudp.loop();

  new_state = myudp.status();
  if (new_state != display_state) {
    if (state_callback) {
      display_state = new_state;
      state_callback(display_state, "");
    }
  }
}

void UdpNet::stop()
{

}

unsigned long UdpNet::get_retry_count()
{
  return myudp.get_retry_count();
}
