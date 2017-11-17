#ifndef APP_BUZZER_H
#define APP_BUZZER_H

#include <Arduino.h>

class Buzzer
{
private:
  uint8_t _pin;
  bool later = false;
  bool active = false;
  unsigned long start_time;
  unsigned int run_time;
public:
  Buzzer(uint8_t pin);
  void loop();
  void chirp();
  void beep(unsigned int time);
  void beep_later(unsigned int time);
};

#endif
