// SPDX-FileCopyrightText: 2020 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CUSTOM_CHARS_H
#define CUSTOM_CHARS_H

uint8_t lcd_char_blank[8] = {
    0, 0, 0, 0, 0, 0, 0, 0
};

uint8_t lcd_char_diamond[8] = {
    0b00000,
    0b00100,
    0b01110,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};

uint8_t lcd_char_backslash[8] = {
    0b00000,
    0b10000,
    0b01000,
    0b00100,
    0b00010,
    0b00001,
    0b00000,
};

uint8_t lcd_char_tick[8] = {
    0b00000,
    0b00001,
    0b00010,
    0b10100,
    0b11000,
    0b10000,
    0b00000
};

uint8_t lcd_char_cross[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b01010,
    0b00100,
    0b01010,
    0b00000
};

uint8_t lcd_char_updown[8] = {
    0b00100,
    0b01110,
    0b11111,
    0b00000,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};

uint8_t lcd_char_middot[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b00100,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

uint8_t lcd_char_lowdot[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00100,
    0b00000
};

uint8_t lcd_char_padlock_closed[] = {
    0b01110,
    0b10001,
    0b10001,
    0b11111,
    0b11011,
    0b11011,
    0b11111,
    0b00000
};

uint8_t lcd_char_padlock_open[] = {
    0b01110,
    0b10000,
    0b10000,
    0b11111,
    0b11011,
    0b11011,
    0b11111,
    0b00000
};

uint8_t lcd_char_pacman1[] = {
    0b01110,
    0b11011,
    0b11110,
    0b11100,
    0b11110,
    0b11111,
    0b01110,
    0b00000
};

uint8_t lcd_char_pacman2[] = {
    0b00000,
    0b01110,
    0b11101,
    0b11111,
    0b11000,
    0b11111,
    0b01110,
    0b00000
};

uint8_t lcd_char_phone_bars[] = {
    0b00001,
    0b00001,
    0b00101,
    0b00101,
    0b10101,
    0b10101,
    0b10101,
    0b00000
};


uint8_t lcd_char_roundspin1[8] = {
    0b00000,
    0b00000,
    0b00000,
    0b10001,
    0b10001,
    0b10001,
    0b00000,
    0b00000
};

uint8_t lcd_char_roundspin2[8] = {
    0b00000,
    0b00000,
    0b01000,
    0b10000,
    0b10001,
    0b00001,
    0b00010,
    0b00000
};

uint8_t lcd_char_roundspin3[8] = {
    0b00000,
    0b00000,
    0b01100,
    0b10000,
    0b00000,
    0b00001,
    0b00110,
    0b00000
};

uint8_t lcd_char_roundspin4[8] = {
    0b00000,
    0b00000,
    0b01110,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b00000
};

uint8_t lcd_char_roundspin5[8] = {
    0b00000,
    0b00000,
    0b00110,
    0b00001,
    0b00000,
    0b10000,
    0b01100,
    0b00000
};

uint8_t lcd_char_roundspin6[8] = {
    0b00000,
    0b00000,
    0b00010,
    0b00001,
    0b10001,
    0b10000,
    0b01000,
    0b00000
};

uint8_t lcd_char_pause[8] = {
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b00000
};

uint8_t lcd_char_play[8] = {
    0b01000,
    0b01100,
    0b01110,
    0b01111,
    0b01110,
    0b01100,
    0b01000,
    0b00000
};

uint8_t lcd_char_connected[8] = {
    0b00100,
    0b00100,
    0b01110,
    0b00000,
    0b01110,
    0b00100,
    0b00100,
    0b00000,
};

uint8_t lcd_char_disconnected[8] = {
    0b00100,
    0b01110,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b00100,
    0b00000,
};

uint8_t lcd_char_ethernet[8] = {
    0b00100,
    0b01010,
    0b10001,
    0b00100,
    0b10001,
    0b01010,
    0b00100,
    0b00000,
};

uint8_t lcd_char_grin[8] = {
    0b00000,
    0b01010,
    0b01010,
    0b00000,
    0b10001,
    0b01110,
    0b00110,
    0b00000
};

uint8_t lcd_char_eh[8] = {
    0b11100,
    0b10000,
    0b11000,
    0b10101,
    0b11101,
    0b00111,
    0b00101,
    0b00101
};

uint8_t lcd_char_file1[8] = {
    0b00111,
    0b01001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b00000
};

uint8_t lcd_char_file2[8] = {
    0b00111,
    0b01001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b00000
};

uint8_t lcd_char_file3[8] = {
    0b00111,
    0b01001,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11111,
    0b00000
};

uint8_t lcd_char_file4[8] = {
    0b00111,
    0b01001,
    0b10001,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000
};

uint8_t lcd_char_file5[8] = {
    0b00111,
    0b01001,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000
};

uint8_t lcd_char_file6[8] = {
    0b00111,
    0b01111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000
};

uint8_t lcd_char_finefile0[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111, 0b00000 };
uint8_t lcd_char_finefile1[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b10001, 0b11001, 0b11111, 0b00000 };
uint8_t lcd_char_finefile2[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b10001, 0b11101, 0b11111, 0b00000 };
uint8_t lcd_char_finefile3[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile4[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b11001, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile5[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b11101, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile6[8] = { 0b00111, 0b01001, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile7[8] = { 0b00111, 0b01001, 0b10001, 0b11001, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile8[8] = { 0b00111, 0b01001, 0b10001, 0b11101, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile9[8] = { 0b00111, 0b01001, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile10[8] = { 0b00111, 0b01001, 0b11001, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile11[8] = { 0b00111, 0b01001, 0b11101, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile12[8] = { 0b00111, 0b01001, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile13[8] = { 0b00111, 0b01101, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };
uint8_t lcd_char_finefile14[8] = { 0b00111, 0b01111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b00000 };

uint8_t *lcd_sequence_finefile[] = {
    lcd_char_finefile0,
    lcd_char_finefile1,
    lcd_char_finefile2,
    lcd_char_finefile3,
    lcd_char_finefile4,
    lcd_char_finefile5,
    lcd_char_finefile6,
    lcd_char_finefile7,
    lcd_char_finefile8,
    lcd_char_finefile9,
    lcd_char_finefile10,
    lcd_char_finefile11,
    lcd_char_finefile12,
    lcd_char_finefile13,
    lcd_char_finefile14
};

uint8_t *lcd_sequence_roundspin[] = {
    lcd_char_roundspin1,
    lcd_char_roundspin2,
    lcd_char_roundspin3,
    lcd_char_roundspin4,
    lcd_char_roundspin5,
    lcd_char_roundspin6
};

uint8_t *lcd_sequence_padlock[] = {
    lcd_char_padlock_closed,
    lcd_char_padlock_open
};

uint8_t *lcd_sequence_pacman[] = {
    lcd_char_pacman1,
    lcd_char_pacman2
};

uint8_t *lcd_sequence_pauseplay[] = {
    lcd_char_pause,
    lcd_char_play
};

uint8_t *lcd_sequence_updown_ani[] = {
    lcd_char_connected,
    lcd_char_disconnected
};

uint8_t *lcd_sequence_file[] = {
    lcd_char_file1,
    lcd_char_file2,
    lcd_char_file3,
    lcd_char_file4,
    lcd_char_file5,
    lcd_char_file6
};

#endif
