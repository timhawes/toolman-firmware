#ifndef APP_NFC_H
#define APP_NFC_H

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

#define MAX_UID_LENGTH 7

struct nfc_token_t {
  uint8_t len;
  uint8_t uid[MAX_UID_LENGTH];
};

typedef void (*nfc_token_present_cb_t)(nfc_token_t token);
typedef void (*nfc_token_removed_cb_t)(void);

class NFC
{
private:
  PN532_I2C *_pn532i2c;
  PN532 *_pn532;
  uint8_t _reset_pin;
  bool ready;
  const uint16_t token_present_timeout = 350;
  const uint16_t pn532_check_interval = 30000;
public:
  nfc_token_present_cb_t token_present_callback = NULL;
  nfc_token_removed_cb_t token_removed_callback = NULL;
  NFC(PN532_I2C &pn532i2c, PN532 &pn532, uint8_t reset_pin);
  void begin();
  void loop();
  //void token_present_callback(nfc_token_present_cb_t cb);
  //void token_removed_callback(nfc_token_removed_cb_t cb);
};

#endif
