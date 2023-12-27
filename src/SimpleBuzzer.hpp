// SPDX-FileCopyrightText: 2018-2023 Tim Hawes
//
// SPDX-License-Identifier: MIT

#ifndef SIMPLEBUZZER_HPP
#define SIMPLEBUZZER_HPP

#include <Arduino.h>
#include <Ticker.h>

class SimpleBuzzer
{
private:
    Ticker ticker;
    bool active = false;
    bool ready = false;
    uint8_t pin;
    unsigned long stop_after;
    void callback();
    void stop();
public:
    SimpleBuzzer(uint8_t pin);
    void begin();
    void beep(unsigned int ms);
    void chirp();
    void click();
};

#endif
