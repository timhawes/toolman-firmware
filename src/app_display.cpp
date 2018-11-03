#include "app_display.h"
#include <Arduino.h>

Display::Display(LiquidCrystal_I2C &lcd)
{
  _lcd = &lcd;
}

void Display::begin()
{
  uint8_t diamond_char[8] = {0, 4, 14, 31, 14, 4, 0};
  uint8_t backslash_char[8] = {0, 16, 8, 4, 2, 1, 0};
  uint8_t tick_char[8] = {0, 0, 1, 2, 20, 24, 16};
  uint8_t cross_char[8] = {0, 0, 0, 10, 4, 10, 0};
  _lcd->begin();
  _lcd->createChar(1, diamond_char);
  _lcd->createChar(2, backslash_char);
  _lcd->createChar(3, tick_char);
  _lcd->createChar(4, cross_char);
  _lcd->clear();
  _lcd->setBacklight(1);
}

void Display::loop()
{
  static unsigned long last_spin;
  static uint8_t spinner_position;
  //const char spinner[] = {124, 47, 45, 2};
  //unsigned int spinner_length = 4;
  const char spinner[] = "0123456789";
  unsigned int spinner_length = 10;

  if (spinner_enabled) {
    if (millis() - last_spin > 100) {
      spinner_position = (spinner_position + 1) % spinner_length;
      _lcd->setCursor(spinner_x, spinner_y);
      _lcd->write(spinner[spinner_position]);
      last_spin = millis();
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
  user_name[10] = ' ';
  user_name[11] = ' ';
  user_name[12] = ' ';
  user_name[13] = 0;
  draw_left(0, 1, name, 13);
  //_lcd->setCursor(0, 1);
  //_lcd->print(name);
  //for (int i=strlen(name)-1; i<19; i++) {
  //  _lcd->write(' ');
  //}
}

void Display::set_device(const char *name)
{
  strncpy(device_name, name, sizeof(device_name));
  draw_left(0, 0, name, 20);
}

void Display::set_network(bool up)
{
  if (!up) {
    message("Network Down");
  }
}

void Display::set_network(bool wifi_up, bool ip_up, bool session_up)
{
  String m = "";

  if (wifi_up) {
    m += "WiFi\x03 ";
  } else {
    m += "WiFi\x04 ";
  }

  if (ip_up) {
    m += "IP\x03 ";
  } else {
    m += "IP\x04 ";
  }

  if (session_up) {
    m += "Session\x03";
  } else {
    m += "Session\x04";
  }

  if (wifi_up && ip_up && session_up) {
    message(m.c_str(), 2000);
  } else {
    message(m.c_str());
  }
}

void Display::set_state(bool enabled, bool active)
{
  if (enabled) {
    if (active) {
      draw_left(0, 0, "Active", 20);
      draw_right(10, 3, "Logout", 10);
    } else {
      draw_left(0, 0, "Ready", 20);
      draw_right(10, 3, "Logout", 10);
    }
  } else { // not enabled
    if (active) {
      // active but not enabled
      draw_left(0, 0, "Waiting for stop", 20);
      draw_right(10, 3, "", 10); // erase logout button
    } else {
      // not enabled and not active
      draw_left(0, 0, device_name, 20);
      draw_left(0, 1, motd, 20); // erase username, show motd
      draw_right(10, 3, "", 10); // erase logout button
    }
  }
}

void Display::set_current(unsigned long milliamps)
{
  char t[10];

  if (!current_enabled) {
    return;
  }

  //dtostrf(milliamps/1000.0, 0, 1, t);
  //t[strlen(t)+1] = 0;
  //t[strlen(t)] = 'A';

  //snprintf(t, sizeof(t), "%d.%dmA", milliamps/1000, milliamps/100);

  snprintf(t, sizeof(t), "%dmA", milliamps);

  draw_left(0, 3, t, 7);
}

void Display::set_motd(const char *_motd)
{
  strncpy(motd, _motd, sizeof(motd));
}

void Display::set_attempts(unsigned int new_attempts)
{
  char t[9];
  snprintf(t, sizeof(t), "%d", new_attempts);
  draw_right(8, 3, t, 3);
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
}

void Display::setup_mode(const char *ssid)
{
  _lcd->clear();
  _lcd->setCursor(0, 1);
  _lcd->print(" Setup Mode");
  _lcd->setCursor(0, 2);
  _lcd->print(" SSID: ");
  _lcd->print(ssid);
}

void Display::firmware_warning()
{
  _lcd->clear();
  _lcd->setCursor(0, 1);
  _lcd->print(" Updating firmware");
  _lcd->setCursor(0, 2);
  _lcd->print(" Do not switch off");
}

void Display::firmware_progress(unsigned int progress)
{
  String m = "Firmware " + String(progress) + "%";
  message(m.c_str());
}

void Display::refresh()
{
  _lcd->clear();
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
