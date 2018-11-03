#include "TimsReliableDatagram.h"

MyUdp::MyUdp()
{
  buf = new cbuf(1024);
  buf->flush();
}

void dns_callback(const char *name, ip_addr_t *ipaddr, void *callback_arg)
{
  if (ipaddr) {
    (*reinterpret_cast<IPAddress*>(callback_arg)) = ipaddr->addr;
    Serial.print("dns: ");
    Serial.print(name);
    Serial.print(" -> ");
    Serial.println((*reinterpret_cast<IPAddress*>(callback_arg)));
  } else {
    Serial.println("dns: lookup failed");
  }
}

bool MyUdp::status()
{
  if (ipaddress && millis() - last_receive < 60000) {
    return true;
  } else {
    return false;
  }
}

void MyUdp::start_lookup()
{
  err_t err = dns_gethostbyname(hostname, &lookup_addr, dns_callback, &ipaddress);
  if (err == ERR_OK) {
    Serial.print("dns: ");
    Serial.print(hostname);
    Serial.print(" -> ");
    Serial.println(ipaddress);
  } else if (err == ERR_INPROGRESS) {
    if (debug_mode) {
      Serial.println("dns: lookup started");
    }
  } else {
    Serial.print("dns: lookup failed err=");
    Serial.println(err);
  }

  last_lookup = millis();
}

unsigned int MyUdp::buf_next_size()
{
  if (buf->available() > 2)
  {
    char size_bytes[2];
    buf->peek((char*)size_bytes, 2);
    return size_bytes[0] << 8 | size_bytes[1];
  } else {
    return 0;
  }
}

bool MyUdp::send(const uint8_t *data, int len)
{
  if (buf->room() > len + 2) {
    buf->write(len >> 8);
    buf->write(len & 255);
    buf->write((const char*)data, len);
    if (debug_mode) {
      Serial.print("queue len=");
      Serial.print(len, DEC);
      Serial.print(" data=");
      Serial.write(data, len);
      Serial.print(" queue=");
      Serial.print(buf->available(), DEC);
      Serial.print("/");
      Serial.println(buf->size(), DEC);
    }
  } else {
    if (debug_mode) {
      Serial.print("queue len=");
      Serial.print(len, DEC);
      Serial.print(" data=");
      Serial.write(data, len);
      Serial.print(" queue=");
      Serial.print(buf->available(), DEC);
      Serial.print("/");
      Serial.print(buf->size(), DEC);
      Serial.println(" full!");
    }
  }

}

void MyUdp::begin(const char *new_hostname, unsigned int new_port)
{
  strncpy(hostname, new_hostname, sizeof(hostname));
  port = new_port;
  local_port = random(1024, 65534);
  client.begin(local_port);
  first_received = false;
  start_lookup();
}

void MyUdp::debug(bool enabled)
{
  debug_mode = enabled;
}

void MyUdp::set_hostname(const char *new_hostname)
{
  strncpy(hostname, new_hostname, sizeof(hostname));
  client.begin(random(1024, 65534));
  start_lookup();
}

void MyUdp::set_port(unsigned int new_port)
{
  port = new_port;
}

void MyUdp::loop()
{

  if (ipaddress) {
    if (millis() - last_lookup > lookup_interval) {
      start_lookup();
    }
  } else {
    if (millis() - last_lookup > lookup_retry) {
      start_lookup();
    }
  }

  int packet_size = client.parsePacket();
  while (packet_size > 0) {
    IPAddress remote_ip = client.remoteIP();
    int remote_port = client.remotePort();
    last_receive = millis();

    if (debug_mode) {
      Serial.print("recv");
    }

    char data[packet_size+1];
    int len = client.read(data, sizeof(data));
    data[len] = 0;

    if (len == 2) {
      uint16_t received_seq = (data[0]<<8) + data[1];
      if (debug_mode) {
        Serial.print(" ack=");
        Serial.println(received_seq, DEC);
      }
      if (buf->available() > 0 && buffer_seq == received_seq) {
        int size = buf_next_size();
        buf->remove(size+2);
        buffer_seq++;
        last_send = millis() - 10000;
        retry_interval = min_retry;
        attempts = 0;
      }
    } else {
      uint16_t received_seq = (data[0]<<8) | data[1];
      int16_t delta = (int16_t)received_seq - (int16_t)last_received_seq;
      if (first_received == false || delta > 0 || (delta < 0 - dup_tolerance)) {
        if (debug_mode) {
          Serial.print(" seq=");
          Serial.print(received_seq, DEC);
          Serial.print(" data=");
          Serial.println((const char*)data + 2);
        }
        first_received = true;
        last_received_seq = received_seq;
        //rx_seq = received_seq;
        client.beginPacket(ipaddress, port);
        client.write((uint8_t)(data[0]));
        client.write((uint8_t)(data[1]));
        client.endPacket();
        if (read_callback) {
          read_callback((const uint8_t*)data + 2, len-2);
        }
        if (fancy_callback) {
          fancy_callback((const uint8_t*)data + 2, len-2);
        }
      } else {
        if (debug_mode) {
          Serial.print(" seq=");
          Serial.print(received_seq, DEC);
          Serial.println(" dup");
        }
        client.beginPacket(ipaddress, port);
        client.write((uint8_t)(data[0]));
        client.write((uint8_t)(data[1]));
        client.endPacket();
      }
    }

    packet_size = client.parsePacket();
  }

  if (buf->available()) {
    unsigned int size = buf_next_size();
    if (size > 0 && ipaddress) {
      if (millis() - last_send > retry_interval) {
        //Serial.print("popping ");
        //Serial.print(size, DEC);
        //Serial.println(" bytes");
        uint8_t msg[size+2];
        buf->peek((char*)msg, size+2);
        if (client.beginPacket(ipaddress, port) == 1) {
          if (client.write((uint8_t)(buffer_seq >> 8)) &&
              client.write((uint8_t)(buffer_seq & 255)) &&
              client.write((uint8_t*)(msg)+2, size) == size) {
            if (client.endPacket()) {
              last_send = millis();
              if (attempts > 0) {
                retry_count++;
              }
              attempts++;
              retry_interval = retry_interval * 1.2;
              if (retry_interval > max_retry) {
                retry_interval = max_retry;
              }
              if (debug_mode) {
                Serial.print("send seq=");
                Serial.print(buffer_seq, DEC);
                Serial.print(" data=");
                Serial.write(msg + 2, size);
                Serial.println();
              }
            }
          }
        }
      }
    }
  }

}

void MyUdp::flush()
{
  buf->flush();
}

unsigned int MyUdp::get_attempts()
{
  return attempts;
}

unsigned long MyUdp::get_retry_count()
{
  return retry_count;
}
