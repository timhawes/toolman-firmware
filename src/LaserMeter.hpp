// SPDX-FileCopyrightText: 2025 Tim Hawes
//
// SPDX-License-Identifier: MIT

#ifndef LASERMETER_HPP
#define LASERMETER_HPP

#include <CRC8.h>
#include <Wire.h>

class LaserMeter
{
private:
    CRC8 crc8;
    TwoWire *i2c_wire;
    uint8_t i2c_address;
    uint64_t total_us = 0; // total microseconds as retrieved
    uint16_t current_ds = 0; // current deciseconds as retrieved
    uint64_t last_us = 0; // last value of total_us that we saw
    uint64_t session_us = 0; // time for this session (resettable)
    bool was_active = false;
    void update_session();
public:
    uint32_t read_crc_errors = 0;
    uint32_t read_i2c_errors = 0;
    uint32_t read_ok = 0;
    LaserMeter(TwoWire &wire, uint8_t address=0x28);
    bool read();
    uint64_t getTotalMicroseconds();
    uint16_t getCurrentDeciseconds();
    bool isActive(); // was active at the point of the last read
    bool wasActive(); // was active between last two reads
    void resetSession();
    uint64_t getSessionMicroseconds();
};

#endif
