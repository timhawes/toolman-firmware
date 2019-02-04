#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include <LiquidCrystal_I2C.h>

class Display
{
private:
  LiquidCrystal_I2C *_lcd;
  char device_name[20];
  char user_name[20];
  char motd[20];
  unsigned long timer_start;
  unsigned long timer_duration;
  uint8_t timer_x;
  uint8_t timer_y;
  unsigned long message_clear_time = 0;
  const uint8_t spinner_x = 19;
  const uint8_t spinner_y = 0;
  bool flashing_enabled = false;
  bool flashing_state = false;
  unsigned long flashing_last_change = 0;
  long flashing_interval = 750;
  unsigned int attempts = 0;
  bool restarting = false;
public:
  Display(LiquidCrystal_I2C &lcd);
  void begin();
  void loop();
  void set_device(const char *name);
  void set_user(const char *name);
  void set_state(bool enabled, bool active);
  void set_current(unsigned int milliamps);
  void set_motd(const char *motd);
  void set_attempts(unsigned int attempts);
  void message(const char *text, unsigned long timeout=0);
  void set_network(bool up);
  void set_network(bool wifi_up, bool ip_up, bool session_up);
  void draw_left(uint8_t x, uint8_t y, const char *text, uint8_t len);
  void draw_right(uint8_t x, uint8_t y, const char *text, uint8_t len);
  void start_timer(uint8_t x, uint8_t y, unsigned long start, unsigned long duration);
  void setup_mode(const char *ssid);
  void restart_warning();
  void firmware_warning();
  void firmware_progress(unsigned int progress);
  void refresh();
  void backlight_on();
  void backlight_off();
  void backlight_flashing();
  bool spinner_enabled = false;
  bool current_enabled = false;
  bool attempts_enabled = false;
  unsigned long active_time;
  unsigned long session_time;
  void draw_clocks();
};

#endif
