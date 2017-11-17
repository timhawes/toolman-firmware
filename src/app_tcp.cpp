#include "app_tcp.h"

TcpNet::TcpNet()
{
}

void TcpNet::begin(config_t &config)
{
  _config = &config;
  wifi_state_connected = false;
  client_connected = false;
  client = new AsyncClient();
  buffer = new ByteBuffer();
  buffer->init(1500);
  buffer->clear();
}

void TcpNet::loop()
{
  static unsigned long count = 0;
  static bool begin_done = false;
  static bool display_state = false;

  run_client();

  if (wifi_state_connected && client->connected()) {
    if (display_state == false) {
      state_callback(true, "TCP up");
      display_state = true;
    }
  } else {
    if (display_state == true) {
      state_callback(false, "TCP down");
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

void TcpNet::abort()
{
  client->abort();
  client->close(true);
}

void TcpNet::wifi_connected()
{
  wifi_state_connected = true;
}

void TcpNet::wifi_disconnected()
{
  wifi_state_connected = false;
}

bool TcpNet::connected()
{
  if (wifi_state_connected && client->connected()) {
    return true;
  } else {
    return false;
  }
}

void TcpNet::run_client()
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

bool TcpNet::send_json(JsonObject &data)
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

  /*
  char send_buffer[1000];
  if (client->canSend()) {
    Serial.print("send: ");
    data.printTo(Serial);
    Serial.println();
    data.printTo(send_buffer, sizeof(send_buffer));
    char lenstring[5];
    snprintf(lenstring, sizeof(lenstring), "%d", data.measureLength());
    size_t written;
    written = client->add(lenstring, strlen(lenstring));
    Serial.print("written ");
    Serial.println(written, DEC);
    written = client->add(send_buffer, data.measureLength());
    Serial.print("written ");
    Serial.println(written, DEC);
    Serial.print("about to send, free heap = ");
    Serial.println(ESP.getFreeHeap(), DEC);
    if (client->send()) {
      Serial.println("sent");
    } else {
      Serial.println("not sent");
    }
    last_send = millis();
    return true;
  } else {
    //Serial.print("cannot send, state=");
    //Serial.println(client->state(), DEC);
    return false;
  }
  */
}

bool TcpNet::buffer_incoming_data(uint8_t *message, uint16_t len)
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

void TcpNet::process_buffered_data()
{

  //while (1) {
    //Serial.print(buffer->getSize(), DEC);
    //Serial.println(" bytes in the buffer");

    if (buffer->getSize() == 0) {
      // no data left in the buffer
      return;
    }

    /*
    Serial.print("buffer> ");
    for (int j=0; j<_network->buffer->getSize(); j++) {
      Serial.write(_network->buffer->peek(j));
    }
    Serial.println();
    yield;
    */

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

bool TcpNet::send(const uint8_t *data, int len)
{
  if (client->canSend()) {
    client->add((const char*)data, len);
    client->send();
  }
  return true;
}
