#include "app_ui.h"

UI::UI(int flash_pin, int a_pin, int b_pin)
{
  button_flash = new Bounce();
  button_a = new Bounce();
  button_b = new Bounce();
  button_flash->attach(flash_pin);
  button_a->attach(a_pin);
  button_b->attach(b_pin);
}

void UI::begin()
{

}

void UI::loop()
{
  button_flash->update();
  button_a->update();
  button_b->update();

  if (button_callback) {
    if (button_flash->rose()) {
      button_callback(0, false);
    }
    if (button_flash->fell()) {
      button_callback(0, true);
    }
    if (button_a->rose()) {
      button_callback(1, false);
    }
    if (button_a->fell()) {
      button_callback(1, true);
    }
    if (button_b->rose()) {
      button_callback(2, false);
    }
    if (button_b->fell()) {
      button_callback(2, true);
    }
  }
}
