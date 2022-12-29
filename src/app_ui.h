// SPDX-FileCopyrightText: 2017-2019 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APP_UI_H
#define APP_UI_H

#include <Bounce2.h>

typedef void (*button_callback_t)(uint8_t button, bool state);

class UI
{
private:
  Bounce *button_flash;
  Bounce *button_a;
  Bounce *button_b;
  int id_a;
  int id_b;
public:
  UI(int flash_pin, int a_pin, int b_pin);
  void begin();
  void loop();
  void swap_buttons(bool swap);
  button_callback_t button_callback;
};

#endif
