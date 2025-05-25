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
    uint64_t total_us = 0;
    uint16_t current_ds = 0;
public:
    uint32_t read_crc_errors = 0;
    uint32_t read_i2c_errors = 0;
    uint32_t read_ok = 0;
    LaserMeter(TwoWire &wire, uint8_t address=0x28);
    bool read();
    uint64_t getTotalMicroseconds();
    uint16_t getCurrentDeciseconds();
    bool isActive();
};

#endif
