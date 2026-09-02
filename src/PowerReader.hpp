// SPDX-FileCopyrightText: 2019-2026 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef POWERREADER_HPP
#define POWERREADER_HPP

#include <Arduino.h>

class PowerReader {
 private:
  // config
  float cal = 1.0; // additional value for making calibration adjustments
  float offset_filter_alpha = 0.001; // smoothing factor for offset tracking
  float ratio = 1.0; // winding ratio of the current-transformer
  float resistor = 1.0; // burden resistor of the current-transformer
  uint8_t adc_pin;
  unsigned long init_time = 60; // sample time for initial DC offset measurement
  unsigned long sample_time = 20; // sample time for actual current measurements
  // state
  float offset = 0; // current DC offset
 public:
  PowerReader(uint8_t _adc_pin);
  float readRMSCurrent();
  float readRMSEquivalentCurrent();
  void begin();
  void setCalibration(float cal);
  void setOffsetAlpha(float alpha);
  void setRatio(float ratio);
  void setResistor(float resistor);
};

#endif
