#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <Hash.h>

#include <ArduinoJson.h>
#include <Buzzer.hpp>
#include <NFCReader.hpp>
#include <base64.hpp>

#include "AppConfig.hpp"
#include "FileWriter.hpp"
#include "FirmwareWriter.hpp"
#include "PowerReader.hpp"
#include "app_display.h"
#include "app_network.h"
#include "app_setup.h"
#include "app_ui.h"
#include "app_util.h"
#include "config.h"
#include "tokendb.hpp"

const uint8_t buzzer_pin = 15;
const uint8_t sda_pin = 13;
const uint8_t scl_pin = 12;
const uint8_t relay_pin = 14;
const uint8_t pn532_reset_pin = 2;
const uint8_t flash_pin = 0;
const uint8_t button_a_pin = 4;
const uint8_t button_b_pin = 5;

char clientid[10];

// config
AppConfig config;

PN532_I2C pn532i2c(Wire);
PN532 pn532(pn532i2c);
NFC nfc(pn532i2c, pn532, pn532_reset_pin);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Display display(lcd);
Network net;
Buzzer buzzer(buzzer_pin);
UI ui(flash_pin, button_a_pin, button_b_pin);

char user_name[20];
char last_user[20];
uint8_t user_access = 0;
char pending_token[15];
unsigned long pending_token_time = 0;
unsigned long last_network_activity = 0;

bool firmware_restart_pending = false;
bool reset_pending = false;
bool restart_pending = false;
bool device_enabled = false; // the relay should be switched on
bool device_relay = false; // the relay *is* switched on
bool device_active = false; // the current sensor is registering a load
unsigned int device_milliamps = 0;

Ticker token_lookup_timer;
Ticker file_timeout_timer;
Ticker firmware_timeout_timer;

MilliClock session_clock;
MilliClock active_clock;
unsigned long session_start;
unsigned long session_went_active;
unsigned long session_went_idle;
bool status_updated = false;

FileWriter file_writer;
FirmwareWriter firmware_writer;
PowerReader power_reader;

buzzer_note network_tune[50];
buzzer_note ascending[] = { {1000, 250}, {1500, 250}, {2000, 250}, {0, 0} };

bool system_is_idle()
{
  return device_enabled == false && device_relay == false && device_active == false;
}

void send_state()
{
  const char *state = "";
  long active_time = 0;
  long idle_time = 0;

  if (device_enabled) {
    if (device_active) {
      state = "active";
      active_time = millis() - session_went_active;
    } else {
      state = "idle";
      idle_time = millis() - session_went_idle;
    }
  } else {
    if (device_active) {
      state = "wait";
      active_time = millis() - session_went_active;
    } else {
      state = "standby";
      strcpy(user_name, "");
    }
  }

  DynamicJsonBuffer jb;
  JsonObject &obj = jb.createObject();
  obj["cmd"] = "state_info";
  obj["state"] = state;
  obj["user"] = user_name;
  obj["milliamps"] = device_milliamps;
  obj["active_time"] = active_time;
  obj["idle_time"] = idle_time;
  net.send_json(obj);

  status_updated = false;
}

void token_info_callback(const char *uid, bool found, const char *name, uint8_t access)
{
  token_lookup_timer.detach();

  Serial.print("token_info_callback: time=");
  Serial.println(millis()-pending_token_time, DEC);

  if (found) {
    if (access > 0) {
      strncpy(user_name, name, sizeof(user_name));
      display.set_user(name);
      device_enabled = true;
      digitalWrite(relay_pin, HIGH);
      device_relay = true;
      status_updated = true;
      session_start = millis();
      session_went_active = session_start;
      session_went_idle = session_start;
      session_clock.reset();
      session_clock.start();
      active_clock.reset();
      display.message("Access Granted", 2000);
      display.set_state(device_enabled, false);
      buzzer.beep(50);
    } else {
      display.message("Access Denied", 2000);
    }
    return;
  }

  TokenDB tokendb("tokens.dat");
  if (tokendb.lookup(uid)) {
    if (tokendb.get_access_level() > 0) {
      strncpy(user_name, tokendb.get_user().c_str(), sizeof(user_name));
      display.set_user(tokendb.get_user().c_str());
      device_enabled = true;
      digitalWrite(relay_pin, HIGH);
      device_relay = true;
      status_updated = true;
      session_start = millis();
      session_went_active = session_start;
      session_went_idle = session_start;
      session_clock.reset();
      session_clock.start();
      active_clock.reset();
      display.message("Access Granted", 2000);
      display.set_state(device_enabled, false);
      buzzer.beep(50);
      return;
    }
  }

  display.message("Access Denied", 2000);
  return;
}

void token_present(NFCToken token)
{
  Serial.print("token_present: ");
  Serial.println(token.uidString());
  buzzer.chirp();
  display.message("Checking...");
  
  DynamicJsonBuffer jb;
  JsonObject &obj = jb.createObject();
  JsonObject &tokenobj = jb.createObject();

  obj["cmd"] = "token_auth";
  if (token.ats_len > 0) {
    tokenobj["ats"] = hexlify(token.ats, token.ats_len);
  }
  tokenobj["atqa"] = (int)token.atqa;
  tokenobj["sak"] = (int)token.sak;
  if (token.version_len > 0) {
    tokenobj["version"] = hexlify(token.version, token.version_len);
  }
  if (token.ntag_counter > 0) {
    tokenobj["ntag_counter"] = (long)token.ntag_counter;
  }
  if (token.ntag_signature_len > 0) {
    tokenobj["ntag_signature"] = hexlify(token.ntag_signature, token.ntag_signature_len);
  }
  if (token.data_len > 0) {
    tokenobj["data"] = hexlify(token.data, token.data_len);
  }
  tokenobj["uid"] = token.uidString();
  obj["uid"] = token.uidString();
  obj["token"] = tokenobj;
  if (token.read_time > 0) {
    obj["read_time"] = token.read_time;
  }
  
  strncpy(pending_token, token.uidString().c_str(), sizeof(pending_token));
  token_lookup_timer.once_ms(config.token_query_timeout, std::bind(&token_info_callback, pending_token, false, "", 0));

  pending_token_time = millis();
  net.send_json(obj);
}

void token_removed(NFCToken token)
{
  Serial.print("token_removed: ");
  Serial.println(token.uidString());
}

void load_config()
{
  config.LoadJson();
  net.set_wifi(config.ssid, config.wpa_password);
  net.set_server(config.server_host, config.server_port, config.server_password,
                 config.server_tls_enabled, config.server_tls_verify,
                 config.server_fingerprint1, config.server_fingerprint2);
  nfc.read_counter = config.nfc_read_counter;
  nfc.read_data = config.nfc_read_data;
  nfc.read_sig = config.nfc_read_sig;
  display.current_enabled = config.dev;
  display.freeheap_enabled = config.dev;
  display.set_device(config.name);
  ui.swap_buttons(config.swap_buttons);
  power_reader.SetInterval(config.adc_interval);
  power_reader.SetAdcAmpRatio(config.adc_multiplier);
}

void send_file_info(const char *filename)
{
  DynamicJsonBuffer jb;
  JsonObject &obj = jb.createObject();
  obj["cmd"] = "file_info";
  obj["filename"] = filename;

  File f = SPIFFS.open(filename, "r");
  if (f) {
    MD5Builder md5;
    md5.begin();
    while (f.available()) {
      uint8_t buf[256];
      size_t buflen;
      buflen = f.readBytes((char*)buf, 256);
      md5.add(buf, buflen);
    }
    md5.calculate();
    obj["size"] = f.size();
    obj["md5"] = md5.toString();
    f.close();
  } else {
    obj["size"] = (char*)NULL;
    obj["md5"] = (char*)NULL;
  }

  net.send_json(obj);
}

void button_callback(uint8_t button, bool state)
{
  static bool button_a;
  static bool button_b;

  switch (button) {
    case 0: {
      // flash button
      if (state) {
        Serial.println("flash button pressed, going into setup mode");
        display.setup_mode(clientid);
        net.stop();
        delay(1000);
        SetupMode setup_mode(clientid, setup_password);
        setup_mode.run();
      }
      break;
    }
    case 1: {
      button_a = state;
      if (state) {
        buzzer.click();
      }
      break;
    }
    case 2: {
      // B button
      button_b = state;
      if (state) {
        buzzer.click();
        if (device_enabled == true) {
          device_enabled = false;
          status_updated = true;
          display.set_state(device_enabled, device_active);
        }
      }
      break;
    }
  }

  if (button_a && button_b && (config.dev || config.reboot_enabled)) {
    display.restart_warning();
    ESP.restart();
    delay(5000);
  }
}

void firmware_status_callback(bool active, bool restart, unsigned int progress)
{
  static unsigned int previous_progress = 0;
  if (previous_progress != progress) {
    Serial.print("firmware install ");
    Serial.print(progress, DEC);
    Serial.println("%");
    previous_progress = progress;
    display.firmware_progress(progress);
  }
  if (restart) {
    firmware_restart_pending = true;
  }
}

void file_timeout_callback()
{
  file_writer.Abort();

  DynamicJsonBuffer jb;
  JsonObject &root = jb.createObject();
  root["cmd"] = "file_write_error";
  root["error"] = "file write timed-out";
  net.send_json(root);
}

void firmware_timeout_callback()
{
  file_writer.Abort();

  DynamicJsonBuffer jb;
  JsonObject &root = jb.createObject();
  root["cmd"] = "firmware_write_error";
  root["error"] = "firmware write timed-out";
  net.send_json(root);
}

void set_file_timeout(bool enabled) {
  if (enabled) {
    file_timeout_timer.once(60, file_timeout_callback);
  } else {
    file_timeout_timer.detach();
  }
}

void set_firmware_timeout(bool enabled) {
  if (enabled) {
    firmware_timeout_timer.once(60, firmware_timeout_callback);
  } else {
    firmware_timeout_timer.detach();
  }
}

void network_state_callback(bool wifi_up, bool tcp_up, bool ready)
{
  display.set_network(wifi_up, tcp_up, ready);
}

/*************************************************************************
 * NETWORK COMMANDS                                                      *
 *************************************************************************/

void network_cmd_buzzer_beep(JsonObject &obj)
{
  if (obj["hz"]) {
    buzzer.beep(obj["ms"].as<int>(), obj["hz"].as<int>());
  } else {
    buzzer.beep(obj["ms"].as<int>());
  }
}

void network_cmd_buzzer_chirp(JsonObject &obj)
{
  buzzer.chirp();
}

void network_cmd_buzzer_click(JsonObject &obj)
{
  buzzer.click();
}

void network_cmd_buzzer_tune(JsonObject &obj)
{
  const char *b64 = obj.get<const char*>("data");
  unsigned int binary_length = decode_base64_length((unsigned char*)b64);
  uint8_t binary[binary_length];
  binary_length = decode_base64((unsigned char*)b64, binary);

  memset(network_tune, 0, sizeof(network_tune));
  if (binary_length > sizeof(network_tune)) {
    memcpy(network_tune, binary, sizeof(network_tune));
  } else {
    memcpy(network_tune, binary, binary_length);
  }

  buzzer.play(network_tune);
}

void network_cmd_display_backlight(JsonObject &obj)
{
  if (obj["backlight"] == "on") {
    display.backlight_on();
  } else if (obj["backlight"] == "off") {
    display.backlight_off();
  } else if (obj["backlight"] == "flashing") {
    display.backlight_flashing();
  }
}

void network_cmd_display_message(JsonObject &obj)
{
  if (obj["timeout"] > 0) {
    display.message(obj["text"], obj["timeout"]);
  } else {
    display.message(obj["text"]);
  }
}

void network_cmd_display_refresh(JsonObject &obj)
{
  display.refresh();
}

void network_cmd_file_data(JsonObject &obj)
{
  const char *b64 = obj.get<const char*>("data");
  unsigned int binary_length = decode_base64_length((unsigned char*)b64);
  uint8_t binary[binary_length];
  binary_length = decode_base64((unsigned char*)b64, binary);

  set_file_timeout(false);

  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  if (file_writer.Add(binary, binary_length, obj["position"])) {
    if (obj["eof"].as<bool>() == 1) {
      if (file_writer.Commit()) {
        // finished and successful
        reply["cmd"] = "file_write_ok";
        reply["filename"] = obj["filename"];
        net.send_json(reply);
        send_file_info(obj["filename"]);
        if (obj["filename"] == "config.json") {
          load_config();
        }
      } else {
        // finished but commit failed
        reply["cmd"] = "file_write_error";
        reply["filename"] = obj["filename"];
        reply["error"] = "file_writer.Commit() failed";
        net.send_json(reply);
      }
    } else {
      // more data required
      reply["cmd"] = "file_continue";
      reply["filename"] = obj["filename"];
      reply["position"] = obj["position"].as<int>() + binary_length;
      net.send_json(reply);
    }
  } else {
    reply["cmd"] = "file_write_error";
    reply["filename"] = obj["filename"];
    reply["error"] = "file_writer.Add() failed";
    net.send_json(reply);
  }

  set_file_timeout(file_writer.Running());
}

void network_cmd_file_delete(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  if (SPIFFS.remove((const char*)obj["filename"])) {
    reply["cmd"] = "file_delete_ok";
    reply["filename"] = obj["filename"];
    net.send_json(reply);
  } else {
    reply["cmd"] = "file_delete_error";
    reply["error"] = "failed to delete file";
    reply["filename"] = obj["filename"];
    net.send_json(reply);
  }
}

void network_cmd_file_dir_query(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();
  JsonArray &files = jb.createArray();
  reply["cmd"] = "file_dir_info";
  reply["path"] = obj["path"];
  if (SPIFFS.exists((const char*)obj["path"])) {
    Dir dir = SPIFFS.openDir((const char*)obj["path"]);
    while (dir.next()) {
      files.add(dir.fileName());
    }
    reply["filenames"] = files;
  } else {
    reply["filenames"] = (char*)NULL;
  }
  net.send_json(reply);
}

void network_cmd_file_query(JsonObject &obj)
{
  send_file_info(obj["filename"]);
}

void network_cmd_file_rename(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  if (SPIFFS.rename((const char*)obj["old_filename"], (const char*)obj["new_filename"])) {
    reply["cmd"] = "file_rename_ok";
    reply["old_filename"] = obj["old_filename"];
    reply["new_filename"] = obj["new_filename"];
    net.send_json(reply);
  } else {
    reply["cmd"] = "file_rename_error";
    reply["error"] = "failed to rename file";
    reply["old_filename"] = obj["old_filename"];
    reply["new_filename"] = obj["new_filename"];
    net.send_json(reply);
  }
}

void network_cmd_file_write(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  set_file_timeout(false);

  if (file_writer.Begin(obj["filename"], obj["md5"], obj["size"])) {
    if (file_writer.UpToDate()) {
        reply["cmd"] = "file_write_error";
        reply["filename"] = obj["filename"];
        reply["error"] = "already up to date";
        net.send_json(reply);
    } else {
      if (file_writer.Open()) {
        reply["cmd"] = "file_continue";
        reply["filename"] = obj["filename"];
        reply["position"] = 0;
        net.send_json(reply);
      } else {
        reply["cmd"] = "file_write_error";
        reply["filename"] = obj["filename"];
        reply["error"] = "file_writer.Open() failed";
        net.send_json(reply);
      }
    }
  } else {
    reply["cmd"] = "file_write_error";
    reply["filename"] = obj["filename"];
    reply["error"] = "file_writer.Begin() failed";
    net.send_json(reply);
  }

  set_file_timeout(file_writer.Running());
}

void network_cmd_firmware_data(JsonObject &obj)
{
  const char *b64 = obj.get<const char*>("data");
  unsigned int binary_length = decode_base64_length((unsigned char*)b64);
  uint8_t binary[binary_length];
  binary_length = decode_base64((unsigned char*)b64, binary);

  set_firmware_timeout(false);

  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  if (firmware_writer.Add(binary, binary_length, obj["position"])) {
    if (obj["eof"].as<bool>() == 1) {
      if (firmware_writer.Commit()) {
        // finished and successful
        reply["cmd"] = "firmware_write_ok";
        net.send_json(reply);
        firmware_status_callback(false, true, 100);
      } else {
        // finished but commit failed
        reply["cmd"] = "firmware_write_error";
        reply["error"] = "firmware_writer.Commit() failed";
        reply["updater_error"] = firmware_writer.GetUpdaterError();
        net.send_json(reply);
        firmware_status_callback(false, false, 0);
      }
    } else {
      // more data required
      reply["cmd"] = "firmware_continue";
      reply["position"] = obj["position"].as<int>() + binary_length;
      net.send_json(reply);
      firmware_status_callback(true, false, firmware_writer.Progress());
    }
  } else {
    reply["cmd"] = "firmware_write_error";
    reply["error"] = "firmware_writer.Add() failed";
    reply["updater_error"] = firmware_writer.GetUpdaterError();
    net.send_json(reply);
    firmware_status_callback(false, false, 0);
  }

  set_firmware_timeout(firmware_writer.Running());
}

void network_cmd_firmware_write(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();

  set_firmware_timeout(false);

  if (firmware_writer.Begin(obj["md5"], obj["size"])) {
    if (firmware_writer.UpToDate()) {
      reply["cmd"] = "firmware_write_error";
      reply["md5"] = obj["md5"];
      reply["error"] = "already up to date";
      reply["updater_error"] = firmware_writer.GetUpdaterError();
      net.send_json(reply);
    } else {
      if (firmware_writer.Open()) {
        reply["cmd"] = "firmware_continue";
        reply["md5"] = obj["md5"];
        reply["position"] = 0;
        net.send_json(reply);
      } else {
        reply["cmd"] = "firmware_write_error";
        reply["md5"] = obj["md5"];
        reply["error"] = "firmware_writer.Open() failed";
        reply["updater_error"] = firmware_writer.GetUpdaterError();
        net.send_json(reply);
      }
    }
  } else {
    reply["cmd"] = "firmware_write_error";
    reply["md5"] = obj["md5"];
    reply["error"] = "firmware_writer.Begin() failed";
    reply["updater_error"] = firmware_writer.GetUpdaterError();
    net.send_json(reply);
  }

  set_firmware_timeout(firmware_writer.Running());
}

void network_cmd_metrics_query(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();
  reply["cmd"] = "metrics_info";
  reply["esp_free_heap"] = ESP.getFreeHeap();
  reply["millis"] = millis();
  reply["net_rx_buf_max"] = net.rx_buffer_high_watermark;
  reply["net_tcp_reconns"] = net.client_reconnections;
  reply["net_tx_buf_max"] = net.tx_buffer_high_watermark;
  reply["net_tx_delay_count"] = net.tx_delay_count;
  reply["net_wifi_reconns"] = net.wifi_reconnections;
  net.send_json(reply);
}

void network_cmd_motd(JsonObject &obj)
{
  display.set_motd(obj["motd"]);
  display.set_state(device_enabled, device_active);
}

void network_cmd_ping(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();
  reply["cmd"] = "pong";
  if (obj["seq"]) {
    reply["seq"] = obj["seq"];
  }
  if (obj["timestamp"]) {
    reply["timestamp"] = obj["timestamp"];
  }
  net.send_json(reply);
}

void network_cmd_reconnect(JsonObject &obj)
{
  net.reconnect(obj["type"]);
}

void network_cmd_reset(JsonObject &obj)
{
  reset_pending = true;
  if (obj["force"]) {
    display.restart_warning();
    ESP.reset();
    delay(5000);
  }
}

void network_cmd_restart(JsonObject &obj)
{
  restart_pending = true;
  if (obj["force"]) {
    display.restart_warning();
    ESP.restart();
    delay(5000);
  }
}

void network_cmd_state_query(JsonObject &obj)
{
  send_state();
}

void network_cmd_stop(JsonObject &obj)
{
  if (device_enabled == true) {
    device_enabled = false;
    status_updated = true;
    display.set_state(device_enabled, device_active);
  }
}

void network_cmd_system_query(JsonObject &obj)
{
  DynamicJsonBuffer jb;
  JsonObject &reply = jb.createObject();
  reply["cmd"] = "system_info";
  reply["esp_free_heap"] = ESP.getFreeHeap();
  reply["esp_chip_id"] = ESP.getChipId();
  reply["esp_sdk_version"] = ESP.getSdkVersion();
  reply["esp_core_version"] = ESP.getCoreVersion();
  reply["esp_boot_version"] = ESP.getBootVersion();
  reply["esp_boot_mode"] = ESP.getBootMode();
  reply["esp_cpu_freq_mhz"] = ESP.getCpuFreqMHz();
  reply["esp_flash_chip_id"] = ESP.getFlashChipId();
  reply["esp_flash_chip_real_size"] = ESP.getFlashChipRealSize();
  reply["esp_flash_chip_size"] = ESP.getFlashChipSize();
  reply["esp_flash_chip_speed"] = ESP.getFlashChipSpeed();
  reply["esp_flash_chip_mode"] = ESP.getFlashChipMode();
  reply["esp_flash_chip_size_by_chip_id"] = ESP.getFlashChipSizeByChipId();
  reply["esp_sketch_size"] = ESP.getSketchSize();
  reply["esp_sketch_md5"] = ESP.getSketchMD5();
  reply["esp_free_sketch_space"] = ESP.getFreeSketchSpace();
  reply["esp_reset_reason"] = ESP.getResetReason();
  reply["esp_reset_info"] = ESP.getResetInfo();
  reply["esp_cycle_count"] = ESP.getCycleCount();
  reply["millis"] = millis();
  net.send_json(reply);
}

void network_cmd_token_info(JsonObject &obj)
{
  token_info_callback(obj["uid"], obj["found"], obj["name"], obj["access"]);
}

void network_message_callback(JsonObject &obj)
{
  String cmd = obj["cmd"];

  last_network_activity = millis();

  if (cmd == "buzzer_beep") {
    network_cmd_buzzer_beep(obj);
  } else if (cmd == "buzzer_chirp") {
    network_cmd_buzzer_chirp(obj);
  } else if (cmd == "buzzer_click") {
    network_cmd_buzzer_click(obj);
  } else if (cmd == "buzzer_tune") {
    network_cmd_buzzer_tune(obj);
  } else if (cmd == "display_backlight") {
    network_cmd_display_backlight(obj);
  } else if (cmd == "display_message") {
    network_cmd_display_message(obj);
  } else if (cmd == "display_refresh") {
    network_cmd_display_refresh(obj);
  } else if (cmd == "file_data") {
    network_cmd_file_data(obj);
  } else if (cmd == "file_delete") {
    network_cmd_file_delete(obj);
  } else if (cmd == "file_dir_query") {
    network_cmd_file_dir_query(obj);
  } else if (cmd == "file_query") {
    network_cmd_file_query(obj);
  } else if (cmd == "file_rename") {
    network_cmd_file_rename(obj);
  } else if (cmd == "file_write") {
    network_cmd_file_write(obj);
  } else if (cmd == "firmware_data") {
    network_cmd_firmware_data(obj);
  } else if (cmd == "firmware_write") {
    network_cmd_firmware_write(obj);
  } else if (cmd == "keepalive") {
    // ignore
  } else if (cmd == "metrics_query") {
    network_cmd_metrics_query(obj);
  } else if (cmd == "motd") {
    network_cmd_motd(obj);
  } else if (cmd == "ping") {
    network_cmd_ping(obj);
  } else if (cmd == "pong") {
    // ignore
  } else if (cmd == "ready") {
    // ignore
  } else if (cmd == "reconnect") {
    network_cmd_reconnect(obj);
  } else if (cmd == "reset") {
    network_cmd_reset(obj);
  } else if (cmd == "restart") {
    network_cmd_restart(obj);
  } else if (cmd == "state_query") {
    network_cmd_state_query(obj);
  } else if (cmd == "stop") {
    network_cmd_stop(obj);
  } else if (cmd == "system_query") {
    network_cmd_system_query(obj);
  } else if (cmd == "token_info") {
    network_cmd_token_info(obj);
  } else {
    DynamicJsonBuffer jb;
    JsonObject &reply = jb.createObject();
    reply["cmd"] = "error";
    reply["requested_cmd"] = reply["cmd"];
    reply["error"] = "not implemented";
    net.send_json(reply);
  }
}

void setup()
{
  pinMode(buzzer_pin, OUTPUT);
  pinMode(pn532_reset_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(button_a_pin, INPUT_PULLUP);
  pinMode(button_b_pin, INPUT_PULLUP);
  pinMode(flash_pin, INPUT);

  digitalWrite(buzzer_pin, LOW);
  digitalWrite(pn532_reset_pin, HIGH);
  digitalWrite(relay_pin, LOW);

  snprintf(clientid, sizeof(clientid), "ss-%06x", ESP.getChipId());
  WiFi.hostname(String(clientid));

  Serial.begin(115200);
  for (int i=0; i<1024; i++) {
    Serial.print(" \b");
  }
  Serial.println();

  Serial.print(clientid);
  Serial.print(" ");
  Serial.println(ESP.getSketchMD5());

  Wire.begin(sda_pin, scl_pin);
  display.begin();
  SPIFFS.begin();

  if (SPIFFS.exists("config.json")) {
    load_config();
  } else {
    Serial.println("config.json is missing, entering setup mode");
    display.setup_mode(clientid);
    net.stop();
    delay(1000);
    SetupMode setup_mode(clientid, setup_password);
    setup_mode.run();
    ESP.restart();
  }

  // run the ADC calculations a few times to stabilise the low-pass filter
  for (int i=0; i<10; i++) {
    power_reader.ReadRmsCurrent();
    yield();
  }

  net.state_callback = network_state_callback;
  net.message_callback = network_message_callback;
  net.begin();

  ui.begin();

  nfc.token_present_callback = token_present;
  nfc.token_removed_callback = token_removed;
  ui.button_callback = button_callback;
}

void adc_loop()
{
  static unsigned long last_read;

  if (config.adc_interval == 0 ||
      config.adc_multiplier == 0 ||
      config.active_threshold == 0) {
    if (device_active == true) {
      device_active = false;
      device_milliamps = 0;
    }
    return;
  }

  if (millis() - last_read > config.adc_interval) {
    device_milliamps = power_reader.ReadRmsCurrent() * 1000;
    display.set_current(device_milliamps);
    last_read = millis();

    if (device_milliamps > config.active_threshold) {
      if (device_enabled && !device_active) {
        // only mark as active is it supposed to be enabled
        // otherwise, it's probably noise
        device_active = true;
        status_updated = true;
        session_went_active = millis();
        active_clock.start();
        display.set_state(device_enabled, device_active);
      }
    } else {
      if (device_active) {
        device_active = false;
        status_updated = true;
        session_went_idle = millis();
        active_clock.stop();
        display.set_state(device_enabled, device_active);
      }
    }
  }
}

void loop() {
  unsigned long loop_start_time;
  unsigned long loop_run_time;

  unsigned long start_time;
  long t_display, t_nfc, t_ui, t_adc;

  loop_start_time = millis();

  if (device_enabled || device_active) {
    display.session_time = session_clock.read();
    display.active_time = active_clock.read();
  }
  display.draw_clocks();

  yield();
  
  start_time = millis();
  nfc.loop();
  t_nfc = millis() - start_time;
  
  yield();
  
  start_time = millis();
  display.loop();
  t_display = millis() - start_time;
  
  yield();
  
  start_time = millis();
  ui.loop();
  t_ui = millis() - start_time;
  
  yield();
  
  start_time = millis();
  adc_loop();
  t_adc = millis() - start_time;
  
  yield();

  if (device_enabled == true && device_active == false && config.idle_timeout != 0) {
    if ((millis() - session_went_idle) > config.idle_timeout) {
      device_enabled = false;
      status_updated = true;
      display.set_state(device_enabled, device_active);
    }
  }

  if (device_relay == true && device_enabled == false && device_active == false) {
    digitalWrite(relay_pin, LOW);
    device_relay = false;
  }

  yield();
  
  if (status_updated) {
    send_state();
  }

  yield();

  loop_run_time = millis() - loop_start_time;
  if (loop_run_time > 56) {
    Serial.print("loop time "); Serial.print(loop_run_time, DEC); Serial.print("ms: ");
    Serial.print("nfc="); Serial.print(t_nfc, DEC); Serial.print("ms ");
    Serial.print("display="); Serial.print(t_display, DEC); Serial.print("ms ");
    Serial.print("ui="); Serial.print(t_ui, DEC); Serial.print("ms ");
    Serial.print("adc="); Serial.print(t_adc, DEC); Serial.println("ms");
  }

  yield();

  if (firmware_restart_pending) {
    if (system_is_idle()) {
      Serial.println("restarting to complete firmware install...");
      net.stop();
      display.firmware_warning();
      delay(1000);
      Serial.println("restarting now!");
      ESP.restart();
      delay(5000);
      display.refresh();
    }
  }

  if (config.network_watchdog_time != 0) {
    if (millis() - last_network_activity > config.network_watchdog_time) {
      if (system_is_idle()) {
        Serial.println("restarting due to network watchdog...");
        net.stop();
        display.firmware_warning();
        delay(1000);
        Serial.println("restarting now!");
        ESP.restart();
        delay(5000);
        display.refresh();
      }
    }
  }

  if (reset_pending || restart_pending) {
    if (system_is_idle()) {
      Serial.println("rebooting at remote request...");
      net.stop();
      display.restart_warning();
      delay(1000);
      if (reset_pending) {
        Serial.println("resetting now!");
        ESP.reset();
      }
      if (restart_pending) {
        Serial.println("restarting now!");
        ESP.restart();
      }
      delay(5000);
      display.refresh();
    }
  }

  yield();

}
