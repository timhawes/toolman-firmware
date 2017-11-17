#include "app_nfc.h"
#include <Arduino.h>

void bytes_to_hexstr(uint8_t bytes[], uint8_t len, char *buffer)
{
  for (int i=0; i<len; i++) {
    int ms = bytes[i] >> 4;
    int ls = bytes[i] & 0x0f;
    char a, b;
    if (ms < 10)
      a = '0' + ms;
    else
      a = 'a' + ms - 10;
    if (ls < 10)
      b = '0' + ls;
    else
      b = 'a' + ls - 10;
    //printf("byte %d is %02x (%x %x %c%c)\n", i, bytes[i], ms, ls, a, b);
    buffer[i*2] = a;
    buffer[(i*2)+1] = b;
  }
  buffer[(len*2)] = 0;
}

NFC::NFC(PN532_I2C &pn532i2c, PN532 &pn532, uint8_t reset_pin)
{
  _pn532i2c = &pn532i2c;
  _pn532 = &pn532;
  _reset_pin = reset_pin;
  ready = false;
}

void NFC::begin()
{
}

void NFC::loop()
{
  uint32_t versiondata;
  static boolean ready = false;
  static unsigned long lastTest = 0;
  static unsigned long lastReset = 0;
  //static uint8_t oldUidLength = 0;
  //static uint8_t oldUid[MAX_UID_LENGTH];
  static nfc_token_t old_token;
  static unsigned long lasttoken = 0;
  //uint8_t newUidLength = 0;
  //uint8_t newUid[MAX_UID_LENGTH];
  nfc_token_t new_token;
  boolean success;
  char uidstring[15];

  //old_token.len = 0;
  //memset(&old_token.uid[0], 0, sizeof(old_token.uid));

  // periodically check that the PN532 is responsive
  if (ready) {
    if (millis() > lastTest + pn532_check_interval) {
      versiondata = _pn532->getFirmwareVersion();
      if (versiondata) {
        lastTest = millis();
      } else {
        Serial.println("PN532 is not responding");
        // unset the ready flag
        ready = false;
      }
    }
  }

  if (!ready) {
    if (millis() - lastReset > pn532_check_interval) {
      Serial.println("Resetting NFC");
      digitalWrite(_reset_pin, LOW);
      delay(10);
      digitalWrite(_reset_pin, HIGH);
      delay(10);
      lastReset = millis();
      _pn532->begin();
      versiondata = _pn532->getFirmwareVersion();
      if (versiondata) {
        Serial.print("PN5");
        Serial.print((versiondata >> 24) & 0xFF, HEX);
        Serial.print(" ");
        Serial.print("V");
        Serial.print((versiondata >> 16) & 0xFF, DEC);
        Serial.print('.');
        Serial.print((versiondata >> 8) & 0xFF, DEC);
        ready = true;
        pn532_check_interval = pn532_check_interval_min;
      } else {
        pn532_check_interval *= 2;
        if (pn532_check_interval > pn532_check_interval_max) {
          pn532_check_interval = pn532_check_interval_max;
        }
        return;
      }
    } else {
      return;
    }

    // Set the max number of retry attempts to read from a token
    // This prevents us from waiting forever for a token, which is
    // the default behaviour of the PN532.
    _pn532->setPassiveActivationRetries(0x00);

    // Call setParameters function in the PN532 and disable the automatic ATS requests.
    //
    // Smart tokens would normally respond to ATS, causing the Arduino I2C buffer limit
    // to be reached and packets to be corrupted.
    uint8_t packet_buffer[64];
    packet_buffer[0] = 0x12;
    packet_buffer[1] = 0x24;
    _pn532i2c->writeCommand(packet_buffer, 2);
    _pn532i2c->readResponse(packet_buffer, sizeof(packet_buffer), 50);

    // configure board to read RFID tags
    _pn532->SAMConfig();

    Serial.println(" ready");
    ready = true;
  }

  // Wait for an ISO14443A type tokens (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = _pn532->readPassiveTargetID(PN532_MIFARE_ISO14443A, &new_token.uid[0], &new_token.len, 100);

  if (success) {
    lasttoken = millis();
    //if ((old_token.len != new_token.len) && (memcmp(&old_token.uid, &new_token.uid, new_token.len) != 0)) {
    if ((old_token.len != new_token.len) || (memcmp(&old_token.uid, &new_token.uid, new_token.len) != 0)) {
      if (token_present_callback) {
        bytes_to_hexstr(new_token.uid, new_token.len, uidstring);
        Serial.print("new token ");
        Serial.println(uidstring);
        token_present_callback(uidstring);
      } else {
        Serial.println("token present");
      }
      old_token = new_token;
    }
  }

  if (old_token.len != 0) {
    if (millis() - lasttoken > token_present_timeout) {
      if (token_removed_callback) {
        token_removed_callback();
      } else {
        Serial.println("token removed");
      }
      //old_token.len = 0;
      //memset(&old_token.uid[0], 0, sizeof(old_token.uid));
      memset(&old_token, 0, sizeof(old_token));
    }
  }

}

/*
void NFC::token_present_callback(nfc_token_present_cb_t cb)
{
  _token_present_callback = cb;
}

void NFC::token_removed_callback(nfc_token_removed_cb_t cb)
{
  _token_removed_callback = cb;
}
*/
