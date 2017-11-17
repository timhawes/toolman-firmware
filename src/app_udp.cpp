#include "app_udp.h"

UdpNet::UdpNet()
{

}

void UdpNet::begin(config_t &config)
{
  _config = &config;
  wifi_state = false;
  client_state = false;
}

void UdpNet::wifi_connected()
{
  wifi_state = true;
  client.begin(random(1024, 65534));
}

void UdpNet::wifi_disconnected()
{
  wifi_state = false;
  client_state = false;
}

bool UdpNet::send(const uint8_t *data, int len)
{
  if ((uint32_t)ipaddress == 0) {
    Serial.println("send: no IP available");
    send_failip_count++;
    return false;
  }
  if (client.beginPacket(ipaddress, _config->server_port) != 1) {
    Serial.println("send: client.beginPacket failed");
    send_failbegin_count++;
    return false;
  }
  if (client.write(data, len) != len) {
    Serial.println("send: client.write failed");
    send_failwrite_count++;
    return false;
  }
  if (!client.endPacket()) {
    Serial.println("client.endPacket failed");
    send_failend_count++;
    return false;
  }
  //Serial.println("send ok");
  return true;
}

bool UdpNet::send_json(JsonObject &data)
{
  char buf[data.measureLength()+1];
  data.printTo(buf, sizeof(buf));
  return send((const uint8_t*)buf, strlen(buf));
}

void dns_callback(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
  if (ipaddr) {
    (*reinterpret_cast<IPAddress*>(callback_arg)) = ipaddr->addr;
    Serial.print("IP address lookup callback: ");
    Serial.print(name);
    Serial.print(" -> ");
    Serial.println((*reinterpret_cast<IPAddress*>(callback_arg)));
  } else {
    Serial.println("IP address lookup callback failed");
  }
}

bool UdpNet::connected()
{
  return wifi_state && client_state;
}

void UdpNet::loop()
{
  static bool display_state = false;
  static unsigned long last_ipaddress_lookup = 0;

  if ((uint32_t)ipaddress == 0 || millis() - last_ipaddress_lookup > 30000) {
    if (millis() - last_ipaddress_lookup > 5000) {
      last_ipaddress_lookup = millis();
      err_t err = dns_gethostbyname(_config->server_host, &lookup_addr, dns_callback, &ipaddress);
      if (err == ERR_OK) {
        ipaddress = lookup_addr.addr;
        Serial.print("IP address lookup ok: ");
        Serial.print(_config->server_host);
        Serial.print(" -> ");
        Serial.println(ipaddress);
      } else if (err == ERR_INPROGRESS) {
        Serial.println("IP address lookup started");
      } else {
        Serial.print("IP address lookup failed: err=");
        Serial.println(err);
      }
    }
  }

  if (wifi_state && !client_state && (uint32_t)ipaddress != 0) {
    if (send((const uint8_t*)"{}", 2)) {
      client_state = true;
    }
  }

  if (wifi_state && client_state) {
    if (display_state == false) {
      state_callback(true, "UDP up");
      display_state = true;
    }
  } else {
    if (display_state == true) {
      state_callback(false, "UDP down");
      display_state = false;
    }
  }

  int packet_size = client.parsePacket();
  while (packet_size > 0) {
    IPAddress remote_ip = client.remoteIP();
    int remote_port = client.remotePort();

    //Serial.print("Packet received size=");
    //Serial.print(packet_size);
    //Serial.print(" from=");
    //Serial.print(remote_ip);
    //Serial.print(":");
    //Serial.print(remote_port);

    char data[packet_size+1];
    int len = client.read(data, sizeof(data));
    data[len] = 0;

    //Serial.print(" data=");
    //Serial.println((const char*)data);

    if (received_json_callback) {
      received_json_callback(data);
    }

    packet_size = client.parsePacket();
  }

}

void UdpNet::stop()
{
  client.stopAll();
}

/*


void Network::loop()
{
  static unsigned long count = 0;
  static bool begin_done = false;
  static bool display_state = false;

  run_client();

  if (wifi_state_connected && client->connected()) {
    if (display_state == false) {
      state_callback(true, "");
      display_state = true;
    }
  } else {
    if (display_state == true) {
      state_callback(false, "");
      display_state = false;
    }
  }

  process_buffered_data();

  if (client->connected()) {
  if (outgoing_packet_len > 0) {
    if (client->canSend()) {
      //Serial.print("sending queued outgoing_packet: ");
      //Serial.println((const char*)outgoing_packet);
      int count = client->add((const char *)outgoing_packet, outgoing_packet_len);
      //Serial.print("bytes added to packet: ");
      //Serial.println(count, DEC);
      if (client->send()) {
        //Serial.println("sent");
      } else {
        Serial.println("not sent");
      }
      outgoing_packet_len = 0;
      last_send = millis();
    }
  }
  }


}

void Network::abort()
{
  client->abort();
  client->close(true);
}

void Network::wifi_connected()
{
  wifi_state_connected = true;
}

void Network::wifi_disconnected()
{
  wifi_state_connected = false;
}

bool Network::connected()
{
  if (wifi_state_connected && client->connected()) {
    return true;
  } else {
    return false;
  }
}

void Network::run_client()
{

  if(client->connected())//client already exists
      return;

  if (!wifi_state_connected) {
    // no wifi
    return;
  }

  if (millis() - last_connect < 1000)
      return;
  last_connect = millis();
  last_receive = millis();

  //Serial.println("beep");

  //client = new AsyncClient();
  //if(!client)//could not allocate client
  //  return;

  client->onError([](void * arg, AsyncClient * client, int error){
    Serial.println("Connect Error");
    //aClient = NULL;
    //delete client;
  }, NULL);

  client->onConnect([=](void * arg, AsyncClient * client2){
    Serial.println("Connected");
    last_receive = millis();
    buffer->clear();
    outgoing_packet_len = 0;

    //if (state_callback) {
    //  state_callback(true, "TCP connected");
    //}

    client2->onError([=](void * arg, AsyncClient * c, int error){
      Serial.print("async error ");
      Serial.println(error, DEC);
    }, NULL);

    //client2->onAck([=](void * arg, AsyncClient * c, size_t len, uint32_t time){
    //  Serial.print("ack ");
    //  Serial.print(len, DEC);
    //  Serial.print(" ");
    //  Serial.println(time, DEC);
    //}, NULL);

    client2->onDisconnect([=](void * arg, AsyncClient * c){
      Serial.println("Disconnected");
      //if (state_callback) {
      //  state_callback(false, "TCP disconnected");
      //}
      //aClient = NULL;
      //delete c;
      //client = NULL;
      //delete c;
      buffer->clear();
      outgoing_packet_len = 0;
    }, NULL);

    client2->onData([=](void * arg, AsyncClient * c, void * data, size_t len){
      //c->ackLater();
      last_receive = millis();
      if ((buffer->getCapacity() - buffer->getSize()) >= len) {
        buffer_incoming_data((uint8_t*)data, len);
        //c->ack(len);
      } else {
        Serial.println("postponing, buffer full");
      }
    }, NULL);
  }, NULL);

  Serial.println("tcp connect...");
  client->setRxTimeout(300);
  if(!client->connect(_config->server_host, _config->server_port)){
    Serial.println("Connect Fail");
    //AsyncClient * client = aClient;
    //aClient = NULL;
    //delete client;
  }

}

bool Network::send_json(JsonObject &data)
{
  if (outgoing_packet_len == 0) {
    //Serial.print("send_json: ");
    //data.printTo(Serial);
    //Serial.println();
    char lenstring[5];
    snprintf(lenstring, sizeof(lenstring), "%d", data.measureLength());
    strncpy((char*)outgoing_packet, lenstring, sizeof(outgoing_packet));
    //Serial.println("printTo");
    char buf[data.measureLength()+1];
    data.printTo(buf, sizeof(buf));

    //data.printTo((char*)(outgoing_packet[strlen(lenstring)]), sizeof(outgoing_packet)-strlen(lenstring));
    //strncpy((char*)(outgoing_packet[strlen(lenstring)]), buf, sizeof(outgoing_packet)-strlen(lenstring));
    // TODO: check data *and* buffer lengths
    for (int i=0; i<data.measureLength(); i++) {
      outgoing_packet[strlen(lenstring) + i] = buf[i];
    }
    outgoing_packet_len = strlen(lenstring) + data.measureLength();
    //Serial.println("queued");
  } else {
    Serial.println("send_json: no space in queue");
  }

}

bool Network::buffer_incoming_data(uint8_t *message, uint16_t len)
{
  if (len == 0) {
    return true;
  }

  //Serial.print("received> ");
  //for (int i=0; i<len; i++) {
  //  Serial.write(message[i]);
  //}
  //Serial.println();

  if ((buffer->getCapacity() - buffer->getSize()) < len) {
    Serial.println("buffer full, postponing");
    return false;
  }

  //Serial.print("buffering ");
  //Serial.print(len);
  //Serial.println(" bytes");
  for (int i=0; i<len; i++) {
    //Serial.print("recv ");
    //Serial.println(message[i], DEC);
    if (!buffer->put(message[i])) {
      Serial.println("error, buffer full");
      client->close();
      return false;
    }
  }

  //Serial.println("accepted into buffer");
  return true;

}

void Network::process_buffered_data()
{

  //while (1) {
    //Serial.print(buffer->getSize(), DEC);
    //Serial.println(" bytes in the buffer");

    if (buffer->getSize() == 0) {
      // no data left in the buffer
      return;
    }

    //Serial.print("buffer> ");
    //for (int j=0; j<_network->buffer->getSize(); j++) {
    //  Serial.write(_network->buffer->peek(j));
    //}
    //Serial.println();
    //yield;

    int max_digits = 5;
    int len = 0;
    int digits = 0;
    char lenstring[max_digits];

    // clear the length string that we're about to build up
    for (int i=0; i<max_digits; i++) {
      lenstring[i] = 0;
    }

    for (int i=0; i<buffer->getSize(); i++) {

      if (i == max_digits) {
        // out of space for the length digits
        Serial.print("error, too many digits for length field: ");
        Serial.println(lenstring);
        abort();
        return;
      }

      if (isDigit(buffer->peek(i))) {
        lenstring[i] = buffer->peek(i);
        digits++;
      } else {
        // no more length digits
        if (digits == 0) {
          Serial.println("error, no length received");
          abort();
          return;
        } else {
          int len = atoi(lenstring);
          //Serial.print("got message with size=");
          //Serial.println(len, DEC);

          if (buffer->getSize() >= len) {
            // there are enough bytes in the buffer to complete the declared payload
            // retrieve the length string and discard
            for (int j=0; j<digits; j++) {
              buffer->get();
            }
            // allocate a buffer for the payload
            uint8_t payload[len+1];
            // copy the payload out of the buffer
            for (int j=0; j<len; j++) {
              payload[j] = buffer->get();
            }
            // add a trailing null to complete the string
            payload[len] = 0;
            //Serial.print("recv: ");
            //Serial.println(payload);

            if (received_message_callback) {
              received_message_callback(payload, len);
            }
            // break from this iteration of the buffer
            // so that we can look at the next length string
            break;
          } else {
            // not enough bytes in the buffer to complete the payload
            //Serial.println("payload is not complete");
            return;
          }
        }
      }

      yield();

    } // end for

    //yield();

  //} // end while

}

void Network::send(const uint8_t *data, int len)
{
  if (client->canSend()) {
    client->add((const char*)data, len);
    client->send();
  }
}

*/
