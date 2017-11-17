#include "app_buzzer.h"

Buzzer::Buzzer(uint8_t pin)
{
  _pin = pin;
  active = false;
}

void Buzzer::loop()
{
  if (active) {
    if (millis() - start_time > run_time) {
      digitalWrite(_pin, LOW);
      active = false;
    }
  }
  if (later) {
    later = false;
    beep(run_time);
  }
}

void Buzzer::chirp()
{
  if (!active) {
    digitalWrite(_pin, HIGH);
    delay(1);
    digitalWrite(_pin, LOW);
  }
}

void Buzzer::beep(unsigned int ms)
{
  if (!active) {
    if (ms <= 50) {
      digitalWrite(_pin, HIGH);
      delay(ms);
      digitalWrite(_pin, LOW);
    } else {
      start_time = millis();
      run_time = ms;
      active = true;
      digitalWrite(_pin, HIGH);
    }
  }
}

void Buzzer::beep_later(unsigned int ms)
{
  if (!active) {
    run_time = ms;
    later = true;
  }
}
