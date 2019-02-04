#ifndef POWERREADER_HPP
#define POWERREADER_HPP

#include <Arduino.h>

typedef void (*powerreader_callback_t)(float voltage, float current, float power);

class PowerReader {
 private:
  float adc_to_amps = 67;
  int interval = 5000;
  float voltage = 240;

 public:
  powerreader_callback_t callback = NULL;
  PowerReader();
  float ReadRmsCurrent();
  float ReadSimpleCurrent();
  void SetAdcAmpRatio(float ratio);
  void SetInterval(int interval);
  void SetVoltage(float voltage);
};

#endif
