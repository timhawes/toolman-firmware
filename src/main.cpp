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
#include "NetThing.hpp"
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

char clientid[15];

// config
AppConfig config;

PN532_I2C pn532i2c(Wire);
PN532 pn532(pn532i2c);
NFC nfc(pn532i2c, pn532, pn532_reset_pin);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Display display(lcd);
NetThing net;
Buzzer buzzer(buzzer_pin);
UI ui(flash_pin, button_a_pin, button_b_pin);

char user_name[20];
char last_user[20];
uint8_t user_access = 0;
char pending_token[15];
unsigned long pending_token_time = 0;

bool firmware_restart_pending = false;
bool reset_pending = false;
bool restart_pending = false;
bool device_enabled = false; // the relay should be switched on
bool device_relay = false; // the relay *is* switched on
bool device_active = false; // the current sensor is registering a load
unsigned int device_milliamps = 0;
unsigned int device_milliamps_simple = 0;

WiFiEventHandler wifiEventConnectHandler;
WiFiEventHandler wifiEventDisconnectHandler;
bool wifi_connected = false;
bool network_connected = false;

Ticker token_lookup_timer;

MilliClock session_clock;
MilliClock active_clock;
unsigned long session_start;
unsigned long session_went_active;
unsigned long session_went_idle;
bool status_updated = false;

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

  StaticJsonDocument<JSON_OBJECT_SIZE(7)> obj;
  obj["cmd"] = "state_info";
  obj["state"] = state;
  obj["user"] = user_name;
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
  token_lookup_timer.once_ms(config.token_query_timeout, std::bind(&token_info_callback, pending_token, false, "", 0));

  pending_token_time = millis();
  obj.shrinkToFit();
  net.sendJson(obj, true);
}

void token_removed(NFCToken token)
{
  Serial.print("token_removed: ");
  Serial.println(token.uidString());
}

void load_config()
{
  config.LoadJson();
  net.setWiFi(config.ssid, config.wpa_password);
  net.setServer(config.server_host, config.server_port,
                config.server_tls_enabled, config.server_tls_verify,
                config.server_fingerprint1, config.server_fingerprint2);
  net.setCred(clientid, config.server_password);
  net.setDebug(config.dev);
  net.setWatchdog(config.net_watchdog_timeout);
  nfc.read_counter = config.nfc_read_counter;
  nfc.read_data = config.nfc_read_data;
  nfc.read_sig = config.nfc_read_sig;
  display.current_enabled = config.dev;
  display.freeheap_enabled = config.dev;
  display.uptime_enabled = config.dev;
  display.set_device(config.name);
  ui.swap_buttons(config.swap_buttons);
  power_reader.SetInterval(config.adc_interval);
  power_reader.SetAdcAmpRatio(config.adc_multiplier);
}

void send_file_info(const char *filename)
{
  StaticJsonDocument<JSON_OBJECT_SIZE(4) + 64> obj;
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

  net.sendJson(obj);
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

void wifi_connect_callback(const WiFiEventStationModeGotIP& event)
{
  wifi_connected = true;
  display.set_network(wifi_connected, network_connected, network_connected);
}

void wifi_disconnect_callback(const WiFiEventStationModeDisconnected& event)
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

void network_restart_callback(bool immediate, bool firmware)
{
  if (immediate) {
    display.restart_warning();
    ESP.reset();
    delay(5000);
  }
  if (firmware) {
    firmware_restart_pending = true;
  } else {
    restart_pending = true;
  }
}

void network_transfer_status_callback(const char *filename, int progress, bool active, bool changed)
{
  static unsigned int previous_progress = 0;
  if (strcmp("firmware", filename) == 0) {
    if (previous_progress != progress) {
      Serial.print("firmware install ");
      Serial.print(progress, DEC);
      Serial.println("%");
      previous_progress = progress;
      display.firmware_progress(progress);
    }
  }
  if (changed && strcmp("config.json", filename) == 0) {
    load_config();
  }
}

/*************************************************************************
 * NETWORK COMMANDS                                                      *
 *************************************************************************/

void network_cmd_buzzer_beep(const JsonDocument &obj)
{
  if (obj["hz"]) {
    buzzer.beep(obj["ms"].as<int>(), obj["hz"].as<int>());
  } else {
    buzzer.beep(obj["ms"].as<int>());
  }
}

void network_cmd_buzzer_chirp(const JsonDocument &obj)
{
  buzzer.chirp();
}

void network_cmd_buzzer_click(const JsonDocument &obj)
{
  buzzer.click();
}

void network_cmd_buzzer_tune(const JsonDocument &obj)
{
  const char *b64 = obj["data"].as<char*>();
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
  }
}

void network_cmd_token_info(const JsonDocument &obj)
{
  token_info_callback(obj["uid"], obj["found"], obj["name"], obj["access"]);
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
  } else if (cmd == "buzzer_tune") {
    network_cmd_buzzer_tune(obj);
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

  snprintf(clientid, sizeof(clientid), "toolman-%06x", ESP.getChipId());
  WiFi.hostname(String(clientid));

  wifiEventConnectHandler = WiFi.onStationModeGotIP(wifi_connect_callback);
  wifiEventDisconnectHandler = WiFi.onStationModeDisconnected(wifi_disconnect_callback);

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
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS.begin() failed");
  }

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

  net.onConnect(network_connect_callback);
  net.onDisconnect(network_disconnect_callback);
  net.onRestartRequest(network_restart_callback);
  net.onReceiveJson(network_message_callback);
  net.onTransferStatus(network_transfer_status_callback);
  net.setCommandKey("cmd");
  net.start();

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
      device_milliamps_simple = 0;
    }
    return;
  }

  if (millis() - last_read > config.adc_interval) {
    device_milliamps = power_reader.ReadRmsCurrent() * 1000;
    device_milliamps_simple = power_reader.ReadSimpleCurrent() * 1000;
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

  if (device_enabled || device_active) {
    display.session_time = session_clock.read();
    display.active_time = active_clock.read();
    display.draw_clocks();
  }

  yield();
  
  nfc.loop();
  
  yield();
  
  display.loop();
  
  yield();
  
  ui.loop();
  
  yield();
  
  adc_loop();
  
  yield();

  net.loop();

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

}
