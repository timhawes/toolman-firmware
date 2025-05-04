// SPDX-FileCopyrightText: 2017-2025 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_display.h"
#include <Arduino.h>
#include "custom_chars.h"

Display::Display(LiquidCrystal_I2C &lcd)
{
  _lcd = &lcd;
}

void Display::begin()
{
  _lcd->begin();
  _lcd->createChar(1, lcd_char_grin);
  //_lcd->createChar(2, backslash_char);
  _lcd->createChar(3, lcd_char_tick);
  _lcd->createChar(4, lcd_char_cross);
  _lcd->createChar(5, lcd_char_middot); // wifi icon
  _lcd->createChar(6, lcd_char_middot); // network connection icon
  _lcd->createChar(7, lcd_char_padlock_closed); // toolman status icon
  _lcd->createChar(8, lcd_char_hourglass);
  _lcd->clear();
  _lcd->setCursor(8, 3);
  _lcd->write(5);
  _lcd->write(6);
  _lcd->write(7);
  _lcd->write(1);
  _lcd->setBacklight(1);
}

void Display::loop()
{
  static unsigned long last_freeheap;
  static unsigned long last_millis;
  static unsigned long last_uptime;
  static unsigned long last_idle;

  if (freeheap_enabled) {
    if (millis() - last_freeheap > 500) {
      char t[6];
      snprintf(t, sizeof(t), "%05d", ESP.getFreeHeap());
      draw_right(7, 2, t, 5);
      last_freeheap = millis();
    }
  }

  if (millis_enabled) {
    if (millis() - last_millis > 500) {
      _lcd->setCursor(0, 2);
      _lcd->print(millis(), DEC);
      last_millis = millis();
    }
  }

  if (uptime_enabled) {
    if (millis() - last_uptime > 1000) {
      unsigned int m = millis();
      if (m < 60000) {
        char t[4];
        snprintf(t, sizeof(t), "%ds", millis() / 1000);
        draw_left(0, 2, t, 5);
      } else if (m < 3600000) {
        char t[4];
        snprintf(t, sizeof(t), "%dm", millis() / 60000);
        draw_left(0, 2, t, 5);
      } else if (m < 86400000) {
        char t[4];
        snprintf(t, sizeof(t), "%dh", millis() / 3600000);
        draw_left(0, 2, t, 5);
      } else {
        char t[7];
        snprintf(t, sizeof(t), "%dd", millis() / 86400000);
        draw_left(0, 2, t, 5);
      }
    }
  }

  if (idle_enabled) {
    if ((long)(millis() - last_idle) > 1000) {
      if (idle_remaining > 0) {
        unsigned long seconds_remaining = (idle_remaining + 999) / 1000;
        if (seconds_remaining < 60) {
          String msg = "\x08" + String(seconds_remaining) + "s";
          draw_left(6, 0, msg.c_str(), 4);
        } else {
          unsigned long minutes_remaining = (idle_remaining + 59999) / 60000;
          if (minutes_remaining < 100) {
            String msg = "\x08" + String(minutes_remaining) + "m";
            draw_left(6, 0, msg.c_str(), 4);
          } else {
            draw_left(6, 0, "", 4);
          }
        }
      }
      last_idle = millis();
    }
  }

  if (timer_duration > 0) {
    long runtime = millis() - timer_start;
    if (runtime < timer_duration) {
      unsigned long remaining = (timer_duration - runtime) / 1000;
      char t[9];
      int minutes = remaining / 60;
      int seconds = remaining % 60;
      snprintf(t, sizeof(t), "%02d:%02d", minutes, seconds);
      draw_right(timer_x, timer_y, t, 8);
    } else {
      timer_duration = 0;
    }
  }

  if (message_clear_time != 0 && millis() > message_clear_time) {
    message("");
  }

  if (flashing_enabled) {
    if (millis() - flashing_last_change > flashing_interval) {
      flashing_state = !flashing_state;
      flashing_last_change = millis();
      _lcd->setBacklight(flashing_state);
    }
  }
}

void Display::set_user(const char *name)
{
  strncpy(user_name, name, sizeof(user_name));
  user_name[sizeof(user_name)-1] = '\0';
  draw_left(0, 1, name, 12);
}

void Display::set_device(const char *name)
{
  strncpy(device_name, name, sizeof(device_name));
  device_name[sizeof(device_name)-1] = '\0';
  draw_left(0, 0, name, 20);
}

void Display::set_network(bool wifi_up, bool tcp_up, bool ready)
{
  if (restarting) {
    // don't show network status on the "Restarting..." screen
    return;
  }

  _lcd->setCursor(8, 3);
  _lcd->write(5);
  _lcd->write(6);

  if (wifi_up) {
    _lcd->createChar(5, lcd_char_phone_bars);
  } else {
    _lcd->createChar(5, lcd_char_middot);
  }

  if (ready) {
    _lcd->createChar(6, lcd_char_connected);
  } else {
    _lcd->createChar(6, lcd_char_middot);
  }
}

void Display::set_state(bool enabled, bool active)
{
  _lcd->setCursor(10, 3);
  _lcd->write(7);
  if (enabled) {
    if (active) {
      _lcd->createChar(7, lcd_char_play);
      draw_left(0, 0, "Active", 12);
      draw_right(12, 3, "Logout", 8);
    } else {
      _lcd->createChar(7, lcd_char_pause);
      draw_left(0, 0, "Ready", 12);
      draw_right(12, 3, "Logout", 8);
    }
  } else { // not enabled
    if (active) {
      // active but not enabled
      _lcd->createChar(7, lcd_char_padlock_open);
      draw_left(0, 0, "Waiting", 12);
      draw_right(12, 3, "", 8); // erase logout button
    } else {
      // not enabled and not active
      _lcd->createChar(7, lcd_char_padlock_closed);
      draw_left(0, 0, device_name, 20);
      draw_left(0, 1, motd, 20); // erase username, show motd
      draw_right(12, 3, "", 8); // erase logout button
    }
  }
}

void Display::set_current(unsigned int milliamps)
{
  char t[10];

  if (!current_enabled) {
    return;
  }

  snprintf(t, sizeof(t), "%umA", milliamps);
  draw_right(13, 2, t, 7);
}

void Display::set_motd(const char *_motd)
{
  strncpy(motd, _motd, sizeof(motd));
  motd[sizeof(motd)-1] = '\0';
}

void Display::message(const char *text, unsigned long timeout)
{
  draw_left(0, 2, text, 20);
  if (timeout) {
    message_clear_time = millis() + timeout;
  } else {
    message_clear_time = 0;
  }
}

void Display::draw_left(uint8_t x, uint8_t y, const char *text, uint8_t len)
{
  uint8_t text_len = strlen(text);
  _lcd->setCursor(x, y);
  for (uint8_t i=0; i<len; i++) {
    if (i < text_len) {
      _lcd->write(text[i]);
    } else {
      _lcd->write(' ');
    }
  }
}

void Display::draw_right(uint8_t x, uint8_t y, const char *text, uint8_t len)
{
  uint8_t text_len = strlen(text);
  _lcd->setCursor(x, y);
  for (uint8_t i=0; i<(len-text_len); i++) {
    _lcd->write(' ');
  }
  for (uint8_t i=0; i<text_len; i++) {
    _lcd->write(text[i]);
  }
}

void Display::start_timer(uint8_t x, uint8_t y, unsigned long start, unsigned long duration)
{
  timer_start = start;
  timer_duration = duration;
  timer_x = x;
  timer_y = y;
}

void Display::restart_warning()
{
  _lcd->clear();
  _lcd->setCursor(0, 1);
  _lcd->print("   Restarting...");
  restarting = true;
}

void Display::setup_mode(const char *ssid)
{
  _lcd->clear();
  _lcd->setCursor(3, 1);
  _lcd->print("Setup Mode");
  _lcd->setCursor(3, 2);
  _lcd->print(ssid);
  restarting = true;
}

void Display::firmware_warning()
{
  _lcd->clear();
  _lcd->setCursor(0, 1);
  _lcd->print(" Updating firmware");
  _lcd->setCursor(0, 2);
  _lcd->print(" Do not switch off");
  restarting = true;
}

void Display::firmware_progress(unsigned int progress)
{
  int frame_count = sizeof(lcd_sequence_finefile) / sizeof(lcd_sequence_finefile[0]);
  int next_frame = (frame_count-1) * progress / 100;
  _lcd->createChar(1, lcd_sequence_finefile[next_frame]);
  char m[14];
}

void Display::refresh()
{
  _lcd->clear();
  restarting = false;
}

void Display::backlight_off()
{
  _lcd->setBacklight(0);
  flashing_state = false;
  flashing_enabled = false;
}

void Display::backlight_on()
{
  _lcd->setBacklight(1);
  flashing_state = true;
  flashing_enabled = false;
}

void Display::backlight_flashing()
{
  if (!flashing_enabled) {
    flashing_state = !flashing_state;
    flashing_enabled = true;
    flashing_last_change = millis();
    _lcd->setBacklight(flashing_state);
  }
}

void Display::draw_clocks()
{
  static unsigned long prev_active_seconds;
  static unsigned long prev_session_seconds;
  unsigned long session_seconds = session_time / 1000;
  unsigned long active_seconds = active_time / 1000;
  if (prev_session_seconds != session_seconds || prev_active_seconds != active_seconds) {
    int s_hours = session_seconds / 3600;
    int s_minutes = (session_seconds / 60) % 60;
    int s_seconds = session_seconds % 60;
    int a_hours = active_seconds / 3600;
    int a_minutes = (active_seconds / 60) % 60;
    int a_seconds = active_seconds % 60;
    char t[10];
    snprintf(t, sizeof(t), "%02d:%02d:%02d", s_hours, s_minutes, s_seconds);
    draw_right(11, 0, t, 9);
    snprintf(t, sizeof(t), "%02d:%02d:%02d", a_hours, a_minutes, a_seconds);
    draw_right(11, 1, t, 9);
    prev_session_seconds = session_seconds;
    prev_active_seconds = active_seconds;
  }
}

void Display::set_char(int position, uint8_t* data) {
  _lcd->createChar(position, data);
}

void Display::set_nfc_state(bool ready) {
  if (ready) {
    _lcd->createChar(1, lcd_char_grin);
  } else {
    _lcd->createChar(1, lcd_char_cross);
  }
}
