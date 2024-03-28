// SPDX-FileCopyrightText: 2017-2024 Tim Hawes
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <FS.h>
#ifdef ESP32
#include <SPIFFS.h>
#endif

#ifdef ESP32
#include <esp_task_wdt.h>
#endif

#include <ArduinoJson.h>
#include <SimpleBuzzer.hpp>
#include <NFCReader.hpp>
#include <base64.hpp>

#include "AppConfig.hpp"
#include "PowerReader.hpp"
#include "app_display.h"
#include "NetThing.hpp"
#include "app_setup.h"
#include "app_ui.h"
#include "app_util.h"
#include "tokendb.hpp"

#ifdef ESP8266
const uint8_t buzzer_pin = 15;
const uint8_t sda_pin = 13;
const uint8_t scl_pin = 12;
const uint8_t relay_pin = 14;
const uint8_t pn532_reset_pin = 2;
const uint8_t flash_pin = 0;
const uint8_t button_a_pin = 4;
const uint8_t button_b_pin = 5;
const uint8_t adc_pin = A0;
#endif
#ifdef ARDUINO_LOLIN_S2_MINI
const uint8_t buzzer_pin = 12;
const uint8_t sda_pin = 11;
const uint8_t scl_pin = 9;
const uint8_t relay_pin = 7;
const uint8_t pn532_reset_pin = 16;
const uint8_t flash_pin = 0;
const uint8_t button_a_pin = 35;
const uint8_t button_b_pin = 33;
const uint8_t adc_pin = 3;
#endif
#ifdef ARDUINO_LOLIN_S3_MINI
const uint8_t buzzer_pin = 10;
const uint8_t sda_pin = 11;
const uint8_t scl_pin = 13;
const uint8_t relay_pin = 12;
const uint8_t pn532_reset_pin = 16;
const uint8_t flash_pin = 0;
const uint8_t button_a_pin = 36;
const uint8_t button_b_pin = 35;
const uint8_t adc_pin = 2;
#endif

char clientid[15];
char hostname[25];

// config
AppConfig config;

PN532_I2C pn532i2c(Wire);
PN532 pn532(pn532i2c);
NFC nfc(pn532i2c, pn532, pn532_reset_pin);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Display display(lcd);
NetThing net(1500, 4096);
SimpleBuzzer buzzer(buzzer_pin);
UI ui(flash_pin, button_a_pin, button_b_pin);
PowerReader power_reader(adc_pin);

char user_name[20];
char last_user[20];
uint8_t user_access = 0;
char pending_token[15];
unsigned long pending_token_time = 0;

bool firmware_restart_pending = false;
bool restart_pending = false;
uint16_t restart_reason = 0;
bool device_enabled = false; // the relay should be switched on
bool device_relay = false; // the relay *is* switched on
bool device_active = false; // the current sensor is registering a load
unsigned int device_milliamps = 0;
unsigned int device_milliamps_simple = 0;

#ifdef ESP8266
WiFiEventHandler wifiEventConnectHandler;
WiFiEventHandler wifiEventDisconnectHandler;
#endif
bool wifi_connected = false;
bool network_connected = false;

Ticker token_lookup_timer;

MilliClock session_clock;
MilliClock active_clock;
unsigned long session_start;
unsigned long session_went_active;
unsigned long session_went_idle;
bool status_updated = false;

#ifdef LOOPMETRICS_ENABLED
LoopMetrics loop_metrics;
#endif

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

  StaticJsonDocument<JSON_OBJECT_SIZE(7)> obj;
  obj["cmd"] = "state_info";
  obj["state"] = state;
  obj["user"] = (const char*)user_name;
  obj["milliamps"] = device_milliamps;
  obj["milliamps_simple"] = device_milliamps_simple;
  obj["active_time"] = active_time;
  obj["idle_time"] = idle_time;
  net.sendJson(obj);

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
      if (config.quiet) {
        buzzer.click();
      } else {
        buzzer.beep(50);
      }
      if (config.events) net.sendEvent("auth", 128, "uid=%s user=%s type=online access=granted", uid, user_name);
    } else {
      display.message("Access Denied", 2000);
      if (config.events) net.sendEvent("auth", 128, "uid=%s user=%s type=online access=denied", uid, name);
    }
    return;
  }

  TokenDB tokendb(TOKENS_FILENAME);
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
      if (config.quiet) {
        buzzer.click();
      } else {
        buzzer.beep(50);
      }
      if (config.events) net.sendEvent("auth", 128, "uid=%s user=%s type=offline access=granted", uid, user_name);
      return;
    }
  }

  display.message("Access Denied", 2000);

  if (config.events) net.sendEvent("auth", 128, "uid=%s user=%s type=offline access=denied", uid, name);

  return;
}

void token_present(NFCToken token)
{
  Serial.print("token_present: ");
  Serial.println(token.uidString());
  if (config.quiet) {
    buzzer.click();
  } else {
    buzzer.chirp();
  }
  display.message("Checking...");
  DynamicJsonDocument obj(512);
  JsonObject tokenobj = obj.createNestedObject("token");

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
  //if (token.data_len > 0) {
  //  tokenobj["data"] = hexlify(token.data, token.data_len);
  //}
  tokenobj["uid"] = token.uidString();

  obj["cmd"] = "token_auth";
  obj["uid"] = token.uidString();
  if (token.read_time > 0) {
    obj["read_time"] = token.read_time;
  }

  strncpy(pending_token, token.uidString().c_str(), sizeof(pending_token));
#ifdef ESP8266
  token_lookup_timer.once_ms(config.token_query_timeout, std::bind(&token_info_callback, pending_token, false, "", 0));
#else
  token_lookup_timer.once_ms(config.token_query_timeout, []() { token_info_callback(pending_token, false, "", 0); });
#endif

  pending_token_time = millis();
  obj.shrinkToFit();
  net.sendJson(obj, true);
  if (config.events) net.sendEvent("token", 64, "uid=%s", pending_token);
}

void token_removed(NFCToken token)
{
  Serial.print("token_removed: ");
  Serial.println(token.uidString());
}

void load_wifi_config()
{
  config.LoadWifiJson();
  net.setWifiCheckInterval(config.wifi_check_interval);
  net.setWiFi(config.ssid, config.wpa_password);
}

void load_net_config()
{
  config.LoadNetJson();
#ifdef ESP8266
  net.setServer(config.server_host, config.server_port,
                config.server_tls_enabled, config.server_tls_verify,
                config.server_fingerprint1, config.server_fingerprint2);
#else
  net.setServer(config.server_host, config.server_port,
                config.server_tls_enabled, config.server_tls_verify,
                config.server_sha256_fingerprint1, config.server_sha256_fingerprint2);
#endif
  net.setCred(clientid, config.server_password);
  net.setConnectionStableTime(config.network_conn_stable_time);
  net.setReconnectMaxTime(config.network_reconnect_max_time);
  net.setReceiveWatchdog(config.network_watchdog_time);
}

void load_app_config()
{
  config.LoadAppJson();
  nfc.read_counter = config.nfc_read_counter;
  nfc.read_data = config.nfc_read_data;
  nfc.read_sig = config.nfc_read_sig;
  nfc.pn532_check_interval = config.nfc_check_interval;
  nfc.pn532_reset_interval = config.nfc_reset_interval;
  nfc.per_5s_limit = config.nfc_5s_limit;
  nfc.per_1m_limit = config.nfc_1m_limit;
  display.current_enabled = config.dev;
  display.freeheap_enabled = config.dev;
  display.uptime_enabled = config.dev;
  display.set_device(config.name);
  ui.swap_buttons(config.swap_buttons);
  power_reader.setCalibration(config.ct_cal);
  power_reader.setRatio(config.ct_ratio);
  power_reader.setResistor(config.ct_resistor);
}

void load_config()
{
  load_wifi_config();
  load_net_config();
  load_app_config();
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
        display.setup_mode(hostname);
        net.stop();
#ifdef ESP8266
        delay(1000);
#else
        delay(500);
#endif
        SetupMode setup_mode(hostname, SETUP_PASSWORD);
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
          if (config.events) net.sendEvent("logout");
        }
      }
      break;
    }
  }

  if (button_a && button_b && (config.dev || config.reboot_enabled)) {
    display.restart_warning();
    net.restartWithReason(NETTHING_RESTART_ENDUSER);
  }
}

#ifdef ESP8266
void wifi_connect_callback(const WiFiEventStationModeGotIP& event)
#else
void wifi_connect_callback(WiFiEvent_t event)
#endif
{
  wifi_connected = true;
  display.set_network(wifi_connected, network_connected, network_connected);
}

#ifdef ESP8266
void wifi_disconnect_callback(const WiFiEventStationModeDisconnected& event)
#else
void wifi_disconnect_callback(WiFiEvent_t event)
#endif
{
  wifi_connected = false;
  display.set_network(wifi_connected, network_connected, network_connected);
}

void network_connect_callback()
{
  network_connected = true;
  display.set_network(wifi_connected, network_connected, network_connected);
}

void network_disconnect_callback()
{
  network_connected = false;
  display.set_network(wifi_connected, network_connected, network_connected);
}

void network_restart_callback(bool immediate, bool firmware, uint16_t reason)
{
  restart_reason = reason;
  if (immediate) {
    display.restart_warning();
    net.restartWithReason(restart_reason);
  }
  if (firmware) {
    firmware_restart_pending = true;
  } else {
    restart_pending = true;
  }
}

void network_transfer_status_callback(const char *filename, int progress, bool active, bool changed)
{
  static int previous_progress = 0;
  if (strcmp("firmware", filename) == 0) {
    if (previous_progress != progress) {
      Serial.print("firmware install ");
      Serial.print(progress, DEC);
      Serial.println("%");
      previous_progress = progress;
      display.firmware_progress(progress);
    }
  }
  if (changed && strcmp(WIFI_JSON_FILENAME, filename) == 0) {
    load_wifi_config();
  }
  if (changed && strcmp(NET_JSON_FILENAME, filename) == 0) {
    load_net_config();
  }
  if (changed && strcmp(APP_JSON_FILENAME, filename) == 0) {
    load_app_config();
  }
}

/*************************************************************************
 * NETWORK COMMANDS                                                      *
 *************************************************************************/

void network_cmd_buzzer_beep(const JsonDocument &obj)
{
  buzzer.beep(obj["ms"].as<int>());
}

void network_cmd_buzzer_chirp(const JsonDocument &obj)
{
  buzzer.chirp();
}

void network_cmd_buzzer_click(const JsonDocument &obj)
{
  buzzer.click();
}

void network_cmd_display_backlight(const JsonDocument &obj)
{
  if (obj["backlight"] == "on") {
    display.backlight_on();
  } else if (obj["backlight"] == "off") {
    display.backlight_off();
  } else if (obj["backlight"] == "flashing") {
    display.backlight_flashing();
  }
}

void network_cmd_display_message(const JsonDocument &obj)
{
  if (obj["timeout"] > 0) {
    display.message(obj["text"], obj["timeout"]);
  } else {
    display.message(obj["text"]);
  }
}

void network_cmd_display_refresh(const JsonDocument &obj)
{
  display.refresh();
}

void network_cmd_metrics_query(const JsonDocument &obj)
{
  DynamicJsonDocument reply(512);
  reply["cmd"] = "metrics_info";
  reply["millis"] = millis();
  reply["nfc_reset_count"] = nfc.reset_count;
  reply["nfc_token_count"] = nfc.token_count;
#ifdef LOOPMETRICS_ENABLED
  reply["loop_delays"] = loop_metrics.over_limit_count;
  reply["loop_average_interval"] = loop_metrics.average_interval;
  reply["loop_last_interval"] = loop_metrics.last_interval;
  reply["loop_max_interval"] = loop_metrics.getAndClearMaxInterval();
#endif
  reply.shrinkToFit();
  net.sendJson(reply);
}

void network_cmd_motd(const JsonDocument &obj)
{
  display.set_motd(obj["motd"]);
  display.set_state(device_enabled, device_active);
}

void network_cmd_state_query(const JsonDocument &obj)
{
  send_state();
}

void network_cmd_stop(const JsonDocument &obj)
{
  if (device_enabled == true) {
    device_enabled = false;
    status_updated = true;
    display.set_state(device_enabled, device_active);
    if (config.events) net.sendEvent("logout");
  }
}

void network_cmd_token_info(const JsonDocument &obj)
{
  token_info_callback(
    obj["uid"],
    obj["found"] | false,
    obj["name"] | "",
    obj["access"] | 0
  );
}

void network_message_callback(const JsonDocument &obj)
{
  String cmd = obj["cmd"];

  if (cmd == "buzzer_beep") {
    network_cmd_buzzer_beep(obj);
  } else if (cmd == "buzzer_chirp") {
    network_cmd_buzzer_chirp(obj);
  } else if (cmd == "buzzer_click") {
    network_cmd_buzzer_click(obj);
  } else if (cmd == "display_backlight") {
    network_cmd_display_backlight(obj);
  } else if (cmd == "display_message") {
    network_cmd_display_message(obj);
  } else if (cmd == "display_refresh") {
    network_cmd_display_refresh(obj);
  } else if (cmd == "metrics_query") {
    network_cmd_metrics_query(obj);
  } else if (cmd == "motd") {
    network_cmd_motd(obj);
  } else if (cmd == "state_query") {
    network_cmd_state_query(obj);
  } else if (cmd == "stop") {
    network_cmd_stop(obj);
  } else if (cmd == "token_info") {
    network_cmd_token_info(obj);
  } else {
    StaticJsonDocument<JSON_OBJECT_SIZE(3)> reply;
    reply["cmd"] = "error";
    reply["requested_cmd"] = obj["cmd"];
    reply["error"] = "not implemented";
    net.sendJson(reply);
  }
}

void nfcreader_status_callback(bool ready) {
  display.set_nfc_state(ready);
}

void setup()
{
  pinMode(buzzer_pin, OUTPUT);
  pinMode(pn532_reset_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  pinMode(button_a_pin, INPUT_PULLUP);
  pinMode(button_b_pin, INPUT_PULLUP);
#ifdef ESP8266
  pinMode(flash_pin, INPUT);
#else
  pinMode(flash_pin, INPUT_PULLUP);
#endif

  digitalWrite(buzzer_pin, LOW);
  digitalWrite(pn532_reset_pin, HIGH);
  digitalWrite(relay_pin, LOW);

#ifdef ESP8266
  snprintf(clientid, sizeof(clientid), "%06x", ESP.getChipId());
  snprintf(hostname, sizeof(hostname), "toolman-%06x", ESP.getChipId());
  wifi_set_sleep_type(NONE_SLEEP_T);
#else
  uint8_t m[6] = {};
  esp_efuse_mac_get_default(m);
  snprintf(clientid, sizeof(clientid), "%02x%02x%02x", m[3], m[4], m[5]);
  snprintf(hostname, sizeof(hostname), "toolman-%02x%02x%02x", m[3], m[4], m[5]);
#endif
  WiFi.hostname(hostname);

#ifdef ESP8266
  wifiEventConnectHandler = WiFi.onStationModeGotIP(wifi_connect_callback);
  wifiEventDisconnectHandler = WiFi.onStationModeDisconnected(wifi_disconnect_callback);
#else
  WiFi.onEvent(wifi_connect_callback, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(wifi_disconnect_callback, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif

#ifdef ESP8266
  Serial.begin(115200);
  for (int i=0; i<1024; i++) {
    Serial.print(" \b");
  }
#else
  Serial.begin();
  Serial.setDebugOutput(false);
#endif
  Serial.println();
  Serial.print(hostname);
  Serial.print(" ");
  Serial.print(ARDUINO_VARIANT);
  Serial.print(" ");
  Serial.println(ESP.getSketchMD5());

  Wire.begin(sda_pin, scl_pin);
  buzzer.begin();
  display.begin();
#ifdef ESP8266
  if (!SPIFFS.begin()) {
#else
  if (!SPIFFS.begin(true)) {
#endif
    Serial.println("SPIFFS.begin() failed");
  }

  if (SPIFFS.exists(WIFI_JSON_FILENAME) && SPIFFS.exists(NET_JSON_FILENAME)) {
    load_config();
  } else {
    Serial.println("config is missing, entering setup mode");
    display.setup_mode(hostname);
    net.stop();
    delay(1000);
    SetupMode setup_mode(hostname, SETUP_PASSWORD);
    setup_mode.run();
    net.restartWithReason(NETTHING_RESTART_CONFIG_CHANGE);
  }

#ifdef ESP32
  enableCore0WDT();
#ifndef CONFIG_FREERTOS_UNICORE
  enableCore1WDT();
#endif
#endif

  power_reader.begin();

  net.onConnect(network_connect_callback);
  net.onDisconnect(network_disconnect_callback);
  net.onRestartRequest(network_restart_callback);
  net.onReceiveJson(network_message_callback);
  net.onTransferStatus(network_transfer_status_callback);
#ifdef ESP32
  net.setKeepalive(35000);
  net.begin();
#endif
  net.start();

  ui.begin();

  nfc.token_present_callback = token_present;
  nfc.token_removed_callback = token_removed;
  nfc.reader_status_callback = nfcreader_status_callback;
  ui.button_callback = button_callback;

#ifdef ESP32
  enableLoopWDT();
#endif
}

void adc_loop()
{
  static unsigned long last_read;

  if (config.adc_interval == 0 ||
      config.active_threshold == 0) {
    if (device_active == true) {
      device_active = false;
      device_milliamps = 0;
      device_milliamps_simple = 0;
    }
    return;
  }

  if ((long)(millis() - last_read) > config.adc_interval) {
    device_milliamps = power_reader.readRMSCurrent() * 1000;
    device_milliamps_simple = power_reader.readRMSEquivalentCurrent() * 1000;
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
        if (config.events) net.sendEvent("active");
      }
    } else {
      if (device_active) {
        device_active = false;
        status_updated = true;
        session_went_idle = millis();
        active_clock.stop();
        display.set_state(device_enabled, device_active);
        if (config.events) net.sendEvent("inactive");
      }
    }
  }
}

void loop() {

  if (device_enabled || device_active) {
    display.session_time = session_clock.read();
    display.active_time = active_clock.read();
    display.draw_clocks();
  }

  nfc.loop();
  display.loop();
  ui.loop();
  adc_loop();
  net.loop();

  if (device_enabled == true && device_active == false && config.idle_timeout != 0) {
    if ((long)(millis() - session_went_idle) > config.idle_timeout) {
      device_enabled = false;
      status_updated = true;
      display.set_state(device_enabled, device_active);
      if (config.events) net.sendEvent("logout");
    }
  }

  if (device_relay == true && device_enabled == false && device_active == false) {
    digitalWrite(relay_pin, LOW);
    device_relay = false;
  }

  if (status_updated) {
    send_state();
  }

  if (firmware_restart_pending) {
    if (system_is_idle()) {
      Serial.println("restarting to complete firmware install...");
      net.stop();
      display.firmware_warning();
      delay(1000);
      Serial.println("restarting now!");
      net.restartWithReason(restart_reason);
    }
  }

  if (restart_pending) {
    if (system_is_idle()) {
      Serial.println("rebooting at remote request...");
      net.stop();
      display.restart_warning();
      delay(1000);
      Serial.println("restarting now!");
      net.restartWithReason(restart_reason);
    }
  }

#ifdef LOOPMETRICS_ENABLED
  loop_metrics.feed();
#endif

}
