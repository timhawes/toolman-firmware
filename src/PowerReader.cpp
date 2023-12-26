// SPDX-FileCopyrightText: 2019-2023 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PowerReader.hpp"

#ifdef ESP8266
uint32_t analogReadMilliVolts(uint8_t pin) {
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
  // D1 Mini has a voltage divider on the ADC input
  return analogRead(pin) * 3200 / 1024;
#else
  // assume default 1V AREF on other ESP8266
  return analogRead(pin) * 1000 / 1024;
#endif
}
#endif

PowerReader::PowerReader(uint8_t _adc_pin) {
  adc_pin = _adc_pin;
}

void PowerReader::begin() {
#ifdef ESP32
  analogSetPinAttenuation(adc_pin, ADC_11db);
#endif

  // initialise the DC offset calculation
  unsigned start_time = millis();
  double total = 0;
  unsigned long count = 0;
  while ((long)(millis() - start_time) < init_time) {
    total += analogReadMilliVolts(adc_pin);
    count++;
  }
  offset = total / count;
}

float PowerReader::readRMSCurrent() {
  int sample_count = 0;
  double total = 0;

  unsigned start_time = millis();
  while ((long)(millis() - start_time) < sample_time) {
    double sample;
    double filtered;

    sample = analogReadMilliVolts(adc_pin);
    sample_count++;

    // low-pass filter to track 0V DC bias
    offset = (offset + (sample - offset) / offset_samples);

    filtered = sample - offset;
    total += (filtered * filtered);
  }

  if (sample_count == 0) {
    // avoid obscure divide-by-zero errors
    // this would only happen if a background task
    // stole the CPU for an extended period
    return 0;
  }

  return sqrt(total / sample_count)  // ADC volts RMS
         * ratio                     // scale for the current-transformer ratio
         * cal                       // apply manual calibration
         / resistor                  // apply Ohm's law (I = V/R)
         / 1000;                     // convert millivolts to volts
}

float PowerReader::readRMSEquivalentCurrent()
{
  const int valcount = 50;
  const int readings = 4;
  uint16_t values[valcount];
  unsigned int max_val = 0;
  unsigned int min_val = -1;

  for (int i=0; i<valcount; i++) {
    values[i] = 0;
    for (int j=0; j<readings; j++) {
      values[i] += analogReadMilliVolts(adc_pin);
    }
  }

  for (int i=0; i<valcount; i++) {
    if (values[i] > max_val) {
      max_val = values[i];
    }
    if (values[i] < min_val) {
      min_val = values[i];
    }
  }

  float vpp = (float)(max_val - min_val) / (float)readings / 1000.0;
  float vrms = vpp / 1.414213562 / 2;
  float amps = cal * ratio * vrms / resistor;
  return amps;
}

void PowerReader::setCalibration(float _cal) {
  cal = _cal;
}

void PowerReader::setRatio(float _ratio) {
  ratio = _ratio;
}

void PowerReader::setResistor(float _resistor) {
  resistor = _resistor;
}
