#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <Arduino.h>

struct queued_message_t {
  unsigned long seq;
  unsigned long last_sent;
  long interval;
  unsigned int attempts;
  unsigned int len;
  uint8_t *data;
};

#endif
