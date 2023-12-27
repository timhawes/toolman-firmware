// SPDX-FileCopyrightText: 2018-2023 Tim Hawes
//
// SPDX-License-Identifier: MIT

#include <SimpleBuzzer.hpp>

SimpleBuzzer::SimpleBuzzer(uint8_t _pin)
{
    pin = _pin;
}

void SimpleBuzzer::begin() {
    pinMode(pin, OUTPUT);
#ifdef ESP8266
    ticker.attach_ms(5, std::bind(&SimpleBuzzer::callback, this));
#else
    ticker.attach_ms(5, +[](SimpleBuzzer* SimpleBuzzer) { SimpleBuzzer->callback(); }, this);
#endif
    ready = true;
}

void SimpleBuzzer::callback()
{
    if (active && (long)(millis() - stop_after) >= 0) {
        stop();
    }
}

void SimpleBuzzer::stop()
{
    digitalWrite(pin, LOW);
    active = false;
}

void SimpleBuzzer::beep(unsigned int ms)
{
    if (!ready) {
        return;
    }
    stop_after = millis() + ms;
    active = true;
    digitalWrite(pin, HIGH);
}

void SimpleBuzzer::chirp()
{
    if (!ready) {
        return;
    }
    if (!active) {
        digitalWrite(pin, HIGH);
        delay(1);
        digitalWrite(pin, LOW);
    }
}

void SimpleBuzzer::click()
{
    if (!ready) {
        return;
    }
    if (!active) {
        digitalWrite(pin, HIGH);
        delayMicroseconds(50);
        digitalWrite(pin, LOW);
    }
}
