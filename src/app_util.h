// SPDX-FileCopyrightText: 2017-2019 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APP_UTIL_H
#define APP_UTIL_H

#include <Arduino.h>

int decode_hex(const char *hexstr, uint8_t *bytes, size_t max_len);
String hexlify(uint8_t bytes[], uint8_t len);
void i2c_scan();

class MilliClock
{
private:
  unsigned long start_time;
  unsigned long running_time;
  bool running;
public:
  MilliClock();
  unsigned long read();
  void start();
  void stop();
  void reset();
};

#ifdef LOOPMETRICS_ENABLED
class LoopMetrics
{
public:
  LoopMetrics();
  int samples = 100;
  float limit_multiplier = 2;
  float average_interval = 0;
  unsigned long last_interval = 0;
  unsigned long max_interval = 0;
  unsigned long over_limit_count = 0;
  void feed();
  unsigned long getAndClearMaxInterval();
};
#endif

#endif
