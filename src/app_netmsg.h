#ifndef APP_NETMSG_H
#define APP_NETMSG_H

#include "FS.h"
#include "app_types.h"
#include "app_network.h"
#include "app_util.h"

typedef void (*token_callback_t)(const char *uid, bool found, const char *name, uint8_t access);
typedef void (*firmware_callback_t)(const char *url, const char *fingerprint, const char *md5);
typedef void (*firmware_status_callback_t)(bool active, bool restart, unsigned int progress);
typedef void (*database_callback_t)(const char *uid, const char *name, uint8_t access);
typedef void (*file_callback_t)(const uint8_t *data, bool eof);
typedef void (*config_callback_t)(const char *key, const JsonVariant& value);
typedef void (*time_callback_t)(time_t time);
typedef void (*network_message_callback_t)(int tag, int len, const uint8_t *value);
typedef void (*netmsg_state_callback_t)(bool up, const char *message);
typedef void (*motd_callback_t)(const char *message);
typedef void (*reboot_callback_t)(bool force);

class NetMsg
{
private:
  config_t *_config;
  Network *_network;
  const char *_clientid;
  void data_cmd_hello(JsonObject &data);
  void data_cmd_bye(JsonObject &data);
  void data_cmd_auth_ok(JsonObject &data);
  void data_cmd_config(JsonObject &data);
  void data_cmd_token_info(JsonObject &data);
  void data_cmd_database_info(JsonObject &data);
  void data_cmd_ping(JsonObject &data);
  void data_cmd_file_list(JsonObject &data);
  void data_cmd_file(JsonObject &data);
  void data_cmd_file_payload(JsonObject &data);
  void data_cmd_firmware(JsonObject &data);
  void data_cmd_firmware_payload(JsonObject &data);
  void data_cmd_motd(JsonObject &data);
  void data_cmd_reboot(JsonObject &data);
  FileWriter *filewriter;
  FirmwareWriter *firmwarewriter;
  unsigned long last_send;
  unsigned long last_receive;
  char pending_token[15]; // max size of a hex-encoded UID
  unsigned long last_token_query;
  unsigned long last_hello;
  unsigned long seq;
  bool connected = false;
  unsigned int data_chunk_size = 512;
  unsigned int max_received_jsonbuffer = 0;
public:
  NetMsg();
  //void send(const uint8_t *data, int len);
  bool send_json(JsonObject &data);
  void send_keepalive();
  void begin(config_t &config, Network &network, const char *clientid);
  void loop();
  void token(const char *uid);
  bool get_file(const char *filename);
  void send_esp_data();
  bool received_message(uint8_t *data, unsigned int len);
  bool receive_json(char *data);
  void network_changed();
  void send_status(const char *status, const char *user, unsigned long milliamps, long active_time, long idle_time);
  void send_status(const char *status, const char *user, unsigned long milliamps, long active_time, long idle_time,
                   unsigned long send_failip_count, unsigned long send_failbegin_count,
                   unsigned long send_failwrite_count, unsigned long send_failend_count);
  token_callback_t token_info_callback = NULL;
  firmware_callback_t firmware_callback = NULL;
  firmware_status_callback_t firmware_status_callback = NULL;
  database_callback_t database_callback = NULL;
  config_callback_t config_callback = NULL;
  time_callback_t time_callback = NULL;
  file_callback_t file_callback = NULL;
  network_message_callback_t network_message_callback = NULL;
  netmsg_state_callback_t state_callback = NULL;
  motd_callback_t motd_callback = NULL;
  reboot_callback_t reboot_callback = NULL;
};

#endif
