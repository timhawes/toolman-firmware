#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <FS.h>

#include "app_network.h"
#include "app_nfc.h"
#include "config.h"

const uint8_t buzzer_pin = 15;
const uint8_t sda_pin = 13;
const uint8_t scl_pin = 12;
const uint8_t relay_pin = 14;
const uint8_t pn532_reset_pin = 2;
const uint8_t button_a_pin = 4;
const uint8_t button_b_pin = 5;

uint8_t diamond_char[8] = {0, 4, 14, 31, 14, 4, 0};

PN532_I2C pn532i2c(Wire);
PN532 pn532(pn532i2c);
NFC nfc(pn532i2c, pn532, pn532_reset_pin);
LiquidCrystal_I2C lcd(0x27, 20, 4);
char clientid[24];

Network net(ssid, wpa_password, server_host, server_port, clientid, "password");

/*
void scan() {
  uint8_t error, address, nDevices;
  Serial.println("scanning i2c devices");
  for(address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknow error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
}
*/

void chirp()
{
  digitalWrite(buzzer_pin, HIGH);
  delay(1);
  digitalWrite(buzzer_pin, LOW);
}

void token_present(nfc_token_t token)
{
  Serial.println("token PRESENT");
  lcd.setCursor(0, 0);
  lcd.print("token present        ");
  lcd.setCursor(0, 1);
  for (uint8_t i=0; i<token.len; i++) {
    if (token.uid[i] < 0x10) {
      lcd.print("0");
    }
    lcd.print(token.uid[i], HEX);
  }
  chirp();
}

void token_removed()
{
  Serial.println("token REMOVED");
  lcd.setCursor(0, 0);
  lcd.print("token removed        ");
  lcd.setCursor(0, 1);
  lcd.print("                    ");
}

void setup()
{
  pinMode(buzzer_pin, OUTPUT);
  pinMode(pn532_reset_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(button_a_pin, INPUT_PULLUP);
  pinMode(button_b_pin, INPUT_PULLUP);

  digitalWrite(buzzer_pin, LOW);
  digitalWrite(pn532_reset_pin, HIGH);
  digitalWrite(relay_pin, LOW);

  Serial.begin(115200);
  delay(500);
  Serial.println("");

  Serial.println("Hello");
  delay(1000);

  snprintf(clientid, sizeof(clientid), "ESP_SS_%08X", ESP.getChipId());

  SPIFFS.begin();
  Wire.begin(sda_pin, scl_pin);
  net.begin();

  lcd.begin();
  lcd.createChar(1, diamond_char);
  lcd.clear();
  lcd.setBacklight(1);
  lcd.setCursor(0, 0);
  lcd.print("Ready");

  lcd.setCursor(0, 3);
  lcd.print("Menu          Logout");

  nfc.token_present_callback = token_present;
  nfc.token_removed_callback = token_removed;
}

void loop() {
  unsigned long loop_start_time;
  unsigned long loop_run_time;

  loop_start_time = millis();

  nfc.loop();
  net.loop();

  loop_run_time = millis() - loop_start_time;
  if (loop_run_time > 23) {
    Serial.print("loop time ");
    Serial.println(loop_run_time, DEC);
  }

  yield();
}
