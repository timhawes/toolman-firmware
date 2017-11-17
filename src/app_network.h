#ifndef APP_NETWORK_H
#define APP_NETWORK_H

#include <ArduinoJson.h>
#include "app_types.h"

typedef void (*network_state_callback_t)(bool up, const char *message);
typedef bool (*received_message_callback_t)(uint8_t *data, unsigned int len);
typedef bool (*received_json_callback_t)(char *data);

class Network
{
public:
  virtual bool send(const uint8_t *data, int len);
  virtual bool send_json(JsonObject &data);
  virtual void begin(config_t &config);
  virtual void loop();
  virtual void wifi_connected();
  virtual void wifi_disconnected();
  virtual bool connected();
  virtual void stop();
  virtual void flush();
  network_state_callback_t state_callback = NULL;
  received_message_callback_t received_message_callback = NULL;
  received_json_callback_t received_json_callback = NULL;
};

#endif
