#include "app_network.h"
#include <ArduinoJson.h>

Network::Network(const char *ssid, const char *wpa_password, const char *host, uint16_t port, const char *clientid, const char *password)
{
  _ssid = ssid;
  _wpa_password = wpa_password;
  _host = host;
  _port = port;
  _clientid = clientid;
  _password = password;
  wifi_connected = false;
  client = new WiFiUDP();
}

void Network::begin()
{
  WiFi.mode(WIFI_STA);
}

void Network::loop()
{
  static unsigned long count = 0;
  static bool begin_done = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifi_connected) {
      Serial.println("wifi connected");
      wifi_connected = true;
      client_connected = false;
      client->begin(10000);
    }
  } else {
    if (wifi_connected) {
      Serial.println("lost wifi");
      wifi_connected = false;
      client_connected = false;
    }
    if (!begin_done) {
      Serial.println("running wifi.begin");
      WiFi.begin(_ssid, _wpa_password);
      begin_done = true;
    }
  }

  udp_receive();

  if (wifi_connected && !client_connected && millis() - last_connect > connect_interval) {
    /*
    if (client->beginPacket(_host, _port)) {
      client->write((uint8_t)0);
      client->write(_clientid, strlen(_clientid));
      client->write((uint8_t)0);
      client->write(_password, strlen(_password));
      client->write((uint8_t)0);
      client->endPacket();
      last_connect = millis();
    }*/
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cmd"] = "hello";
    root["client"] = _clientid;
    root["password"] = _password;
    root.printTo(send_buffer, sizeof(send_buffer));
    send_len = root.measureLength();
    udp_send();
  }

  /*
  if (client_connected) {
    if (millis() - last_send > 1000) {
      client->write("PING\r\n");
      last_send = millis();
    }
    if (millis() - last_receive > 10000) {
      Serial.println("no data received recently, disconnecting");
      client->close();
      last_receive = millis();
    }
  }
  */

  //Serial.println(client.state(), DEC);
}

void Network::udp_send()
{
  if (send_len > 0) {
    Serial.print("udp_send: ");
    Serial.println(send_buffer);
    if (client->beginPacket(_host, _port)) {
      client->write(send_buffer, send_len);
      if (client->endPacket()) {
        Serial.println("sent");
        send_len = 0;
      } else {
        Serial.println("failed");
      }
    }
  }
}

void Network::udp_receive()
{
  uint8_t message[1500];
  uint16_t len = 0;
  uint16_t packet_len;
  IPAddress remote_ip;
  uint16_t remote_port;

  while (1) {
    packet_len = client->parsePacket();
    if (packet_len > 0) {
      remote_ip = client->remoteIP();
      remote_port = client->remotePort();

      Serial.print("Packet received size=");
      Serial.print(packet_len);
      Serial.print(" from=");
      Serial.print(remote_ip);
      Serial.print(":");
      Serial.print(remote_port);

      len = client->read(message, sizeof(message));
      if (len > 0) {
        Serial.print(" data=");
        for (int i = 0; i < len; i++) {
          if (message[i] < 16) {
            Serial.print('0');
          }
          Serial.print(message[i], HEX);
          Serial.print(' ');
        }
      }
      Serial.println();
      if (len > 0) {
        incoming_handler(message, len);
      }
    } else {
      break;
    }
  }

}

void Network::incoming_handler(uint8_t *message, uint16_t len)
{

}
