// SPDX-FileCopyrightText: 2025 Tim Hawes
//
// SPDX-License-Identifier: MIT

#include <LaserMeter.hpp>

LaserMeter::LaserMeter(TwoWire &wire, uint8_t address)
{
    i2c_wire = &wire;
    i2c_address = address;
    crc8.begin();
}

bool LaserMeter::read()
{
    i2c_wire->beginTransmission(i2c_address);
    i2c_wire->write(0x0D);
    i2c_wire->endTransmission();

    i2c_wire->requestFrom(i2c_address, 11);

    if (i2c_wire->available() == 11) {
        uint8_t buffer[11];
        for (uint8_t i = 0; i < 11; i++) {
            buffer[i] = Wire.read();
        }
        // for (uint8_t i = 0; i < 11; i++) {
        //     if (buffer[i] <= 0x0F) {
        //         Serial.print("0");
        //     }
        //     Serial.print(buffer[i], HEX);
        // }
        // Serial.println("");
        uint64_t new_total_us = 0;
        for (uint8_t i = 0; i < 8; i++) {
            new_total_us |= ((uint64_t)buffer[i]) << (8*i);
        }
        uint16_t new_current_ds = 0;
        for (uint8_t i = 0; i < 2; i++) {
            new_current_ds |= ((uint16_t)buffer[i+8]) << (8*i);
        }
        uint8_t new_crc = buffer[10];
        uint8_t calculated_crc = crc8.get_crc8(buffer, 10) ^ 0x55;
        if (new_crc == calculated_crc) {
            total_us = new_total_us;
            current_ds = new_current_ds;
            read_ok++;
            update_session();
            return true;
        } else {
            // crc failed
            read_crc_errors++;
            return false;
        }
    } else {
        // i2c incorrect number of bytes available
        read_i2c_errors++;
        return false;
    }
}

void LaserMeter::update_session()
{
    int64_t since_last_update = total_us - last_us;
    last_us = total_us;
    if (since_last_update > 0) {
        session_us += since_last_update;
    }
}

uint64_t LaserMeter::getTotalMicroseconds()
{
    return total_us;
}

uint16_t LaserMeter::getCurrentDeciseconds()
{
    return current_ds;
}

bool LaserMeter::isActive()
{
    if (current_ds != 65535) {
        return true;
    } else {
        return false;
    }
}

void LaserMeter::resetSession()
{
    session_us = 0;
}

uint64_t LaserMeter::getSessionMicroseconds()
{
    return session_us;
}
