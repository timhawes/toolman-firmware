//#define ASYNC_TCP_SSL_ENABLED 1
//#define DEBUG_ESP_HTTP_CLIENT 1
//#define DEBUG_ESP_PORT Serial

#define APP_UDP_MODE 1
#define APP_UDP2_MODE 1

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <FS.h>
#include <ESP8266httpUpdate.h>
#include <Time.h>
#include <Hash.h>

#include "config.h"
#include "app_types.h"
#include "app_buzzer.h"
#include "app_display.h"
#include "app_wifi.h"

#ifdef APP_UDP_MODE
#include "app_udp.h"
#else
#include "app_tcp.h"
#endif

#include "app_netmsg.h"
#include "app_nfc.h"
#include "app_setup.h"
#include "app_ui.h"

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
config_t config;

// firmware url for OTA update
char firmware_url[255] = "";
char firmware_fingerprint[255] = "";
char firmware_md5[33] = "";
bool firmware_available = false;

PN532_I2C pn532i2c(Wire);
PN532 pn532(pn532i2c);
NFC nfc(pn532i2c, pn532, pn532_reset_pin);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Display display(lcd);
#ifdef APP_UDP_MODE
UdpNet net;
#else
TcpNet net;
#endif
NetMsg netmsg;
WifiSupervisor wifisupervisor;
Buzzer buzzer(buzzer_pin);
UI ui(flash_pin, button_a_pin, button_b_pin);

char user_name[20];
uint8_t user_access = 0;

bool firmware_restart_pending = false;
bool reboot_pending = false;
bool device_enabled = false; // the relay should be switched on
bool device_relay = false; // the relay *is* switched on
bool device_active = false; // the current sensor is registering a load
unsigned long device_milliamps = 0;

bool network_state_wifi = false;
bool network_state_ip = false;
bool network_state_session = false;

MilliClock session_clock;
MilliClock active_clock;
unsigned long session_start;
unsigned long session_went_active;
unsigned long session_went_idle;
bool status_updated = false;

String file_name;
File file_handle;

void i2c_scan() {
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
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
}

void token_present(const char *uid)
{
  buzzer.chirp();
  display.message("Checking...");
  netmsg.token(uid);
}

void token_removed()
{

}

bool token_lookup(const char *uid)
{
  uint8_t uidbytes[7];
  uint8_t uidlen;

  uidlen = decode_hex((const char*)uid, uidbytes, sizeof(uidbytes));

  //Serial.print("looking for ");
  //Serial.print(uidlen, DEC);
  //Serial.println(" bytes UID in tokens files");

  //Serial.print(uidlen, HEX);
  //for (int i=0; i<uidlen; i++) {
  //  Serial.print(" ");
  //  Serial.print(uidbytes[i], HEX);
  //}
  //Serial.println();
  //Serial.println("---");

  if (SPIFFS.exists("tokens")) {
    File tokens_file = SPIFFS.open("tokens", "r");
    if (tokens_file) {
      uint8_t dbversion = tokens_file.read();
      if (dbversion == 1) {
        while (tokens_file.available()) {
          uint8_t xlen = tokens_file.read();
          uint8_t xuid[xlen];
          //Serial.print(xlen, HEX);
          for (int i=0; i<xlen; i++) {
            xuid[i] = tokens_file.read();
            //Serial.print(" ");
            //Serial.print(xuid[i], HEX);
          }
          //Serial.println();
          if ((xlen == uidlen) && (memcmp(xuid, uidbytes, xlen) == 0)) {
            // found
            //Serial.println("found!");
            tokens_file.close();
            return true;
          //} else {
          //  Serial.println("nope");
          }
        }
      } else {
        Serial.print("unknown tokens version ");
        Serial.println(dbversion, DEC);
      }
    } else {
      Serial.println("unable to open tokens file");
    }
  } else {
    Serial.println("tokens file not found");
  }

  return false;
}

void token_info_callback(const char *uid, bool found, const char *name, uint8_t access)
{
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
      buzzer.beep_later(50);
    } else {
      display.message("Access Denied", 2000);
    }
    return;
  }

  if (token_lookup(uid)) {
    //strncpy(user_name, "[unknown]", sizeof(user_name));
    strncpy(user_name, uid, sizeof(user_name));
    display.set_user(uid);
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
    buzzer.beep_later(50);
    return;
  }

  display.message("Access Denied", 2000);
  return;
}

void file_callback(const uint8_t *data, bool eof)
{
  //if (eof) {
  //  Serial.println("file-eof");
  //  if (file_handle) {
  //    Serial.println("closing file");
  //    file_handle.close();
  //  }
  //} else {
  //  Serial.println("file-data");
  //  //Serial.println(data);
  //  //Serial.println(".");
  //  if (file_handle) {
  //    file_handle.write(data);
  //  }
  //}
}

void device_name_callback(const char *name)
{
  display.set_device(name);
}

void wifi_connected_callback()
{
  //display.message("WiFi up", 2000);
  network_state_wifi = true;
  display.set_network(network_state_wifi, network_state_ip, network_state_session);
  net.wifi_connected();
}

void wifi_disconnected_callback()
{
  network_state_wifi = false;
  display.set_network(network_state_wifi, network_state_ip, network_state_session);
  net.wifi_disconnected();
}

void network_state_callback(bool up, const char *text)
{
  if (up) {
    netmsg.network_changed();
    network_state_ip = true;
    display.set_network(network_state_wifi, network_state_ip, network_state_session);
    //display.message(text, 2000);
  } else {
    //display.message(text);
    network_state_ip = false;
    display.set_network(network_state_wifi, network_state_ip, network_state_session);
  }
}

void netmsg_state_callback(bool up, const char *text)
{
  if (up) {
    network_state_session = true;
    display.set_network(network_state_wifi, network_state_ip, network_state_session);
  } else {
    network_state_session = false;
    display.set_network(network_state_wifi, network_state_ip, network_state_session);
  }
}

void hexdump_bytes(uint8_t buffer[], int len)
{
  for (int i=0; i<len; i++) {
    if (buffer[i] < 16) {
      Serial.print("0");
    }
    Serial.print(buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void time_callback(time_t t)
{
  setTime(t);
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
        SetupMode setup_mode(clientid, setup_password);
        setup_mode.run();
      }
      break;
    }
    case 1: {
      button_a = state;
      break;
    }
    case 2: {
      // B button
      button_b = state;
      if (state) {
        if (device_enabled == true) {
          device_enabled = false;
          status_updated = true;
          display.set_state(device_enabled, device_active);
        }
      }
      break;
    }
  }

  if (button_a && button_b && config.dev) {
    display.restart_warning();
    ESP.restart();
  }
}

void load_config()
{
  // defaults
  strncpy(config.server_host, default_server_host, sizeof(config.server_host));
  config.server_port = default_server_port;
  strncpy(config.server_password, default_server_password, sizeof(config.server_password));
  config.token_query_timeout = 500;
  config.report_status_interval = 30000;
  config.net_keepalive_interval = 30000;

  if (SPIFFS.exists("config.json")) {
    File configFile = SPIFFS.open("config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (json.success()) {
        Serial.print("configuration loaded ");
        json.printTo(Serial);
        Serial.println();
        if (json.containsKey("ssid")) {
          strncpy(config.ssid, json["ssid"], sizeof(config.ssid));
        }
        if (json.containsKey("wpa_password")) {
          strncpy(config.wpa_password, json["wpa_password"], sizeof(config.wpa_password));
        }
        if (json.containsKey("server_host")) {
          strncpy(config.server_host, json["server_host"], sizeof(config.server_host));
        }
        if (json.containsKey("server_port")) {
          config.server_port = json["server_port"];
        }
        if (json.containsKey("server_password")) {
          strncpy(config.server_password, json["server_password"], sizeof(config.server_password));
        }
        if (json.containsKey("name")) {
          strncpy(config.name, json["name"], sizeof(config.name));
        }
        if (json.containsKey("idle_timeout")) {
          config.idle_timeout = json["idle_timeout"];
        }
        if (json.containsKey("active_timeout")) {
          config.active_timeout = json["active_timeout"];
        }
        if (json.containsKey("adc_multiplier")) {
          config.adc_multiplier = json["adc_multiplier"];
        }
        if (json.containsKey("adc_interval")) {
          config.adc_interval = json["adc_interval"];
        }
        if (json.containsKey("active_threshold")) {
          config.active_threshold = json["active_threshold"];
        }
        if (json.containsKey("card_present_timeout")) {
          config.card_present_timeout = json["card_present_timeout"];
        }
        if (json.containsKey("net_keepalive_interval")) {
          config.net_keepalive_interval = json["net_keepalive_interval"];
        }
        if (json.containsKey("net_retry_min_interval")) {
          config.net_retry_min_interval = json["net_retry_min_interval"];
        }
        if (json.containsKey("net_retry_max_interval")) {
          config.net_retry_max_interval = json["net_retry_max_interval"];
        }
        if (json.containsKey("long_press_time")) {
          config.long_press_time = json["long_press_time"];
        }
        if (json.containsKey("report_status_interval")) {
          config.report_status_interval = json["report_status_interval"];
        }
        if (json.containsKey("token_query_timeout")) {
          config.token_query_timeout = json["token_query_timeout"];
        }
        if (json.containsKey("dev")) {
          config.dev = json["dev"];
        }
      }
    }
  }
}

void save_config()
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["ssid"] = config.ssid;
  json["wpa_password"] = config.wpa_password;
  json["server_host"] = config.server_host;
  json["server_port"] = config.server_port;
  json["server_password"] = config.server_password;
  json["name"] = config.name;
  json["idle_timeout"] = config.idle_timeout;
  json["active_timeout"] = config.active_timeout;
  json["adc_multiplier"] = config.adc_multiplier;
  json["adc_interval"] = config.adc_interval;
  json["active_threshold"] = config.active_threshold;
  json["card_present_timeout"] = config.card_present_timeout;
  json["net_keepalive_interval"] = config.net_keepalive_interval;
  json["net_retry_min_interval"] = config.net_retry_min_interval;
  json["net_retry_max_interval"] = config.net_retry_max_interval;
  json["long_press_time"] = config.long_press_time;
  json["report_status_interval"] = config.report_status_interval;
  json["token_query_timeout"] = config.token_query_timeout;
  json["dev"] = config.dev;
  File configFile = SPIFFS.open("config.json", "w");
  if (configFile) {
    json.printTo(configFile);
    configFile.close();
    Serial.println("configuration saved");
  } else {
    Serial.println("configuration could not be saved");
  }
  config.changed = false;
}

void firmware_callback(const char *url, const char *fingerprint, const char *md5)
{
  String current_md5 = ESP.getSketchMD5();
  if (strncmp(current_md5.c_str(), md5, 32) == 0) {
    Serial.println("firmware is up to date");
  } else {
    Serial.println("new firmware is available");
    strncpy(firmware_url, url, sizeof(firmware_url));
    strncpy(firmware_fingerprint, fingerprint, sizeof(firmware_fingerprint));
    strncpy(firmware_md5, md5, sizeof(firmware_md5));
    firmware_available = true;
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

bool received_json_callback(char *data)
{
  return netmsg.receive_json(data);
}

void config_callback(const char *key, const JsonVariant& value)
{
  //Serial.print("config callback ");
  //Serial.print(key);
  //Serial.print("=");
  //Serial.println((const char*)value);
  if (strcmp(key, "ssid") == 0) {
    if (strncmp(value, config.ssid, sizeof(config.ssid)) != 0) {
      strncpy(config.ssid, value, sizeof(config.ssid));
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "wpa_password") == 0) {
    if (strncmp(value, config.wpa_password, sizeof(config.wpa_password)) != 0) {
      strncpy(config.wpa_password, value, sizeof(config.wpa_password));
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "server_host") == 0) {
    if (strncmp(value, config.server_host, sizeof(config.server_host)) != 0) {
      strncpy(config.server_host, value, sizeof(config.server_host));
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "server_port") == 0) {
    if (value != config.server_port) {
      config.server_port = value;
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "server_password") == 0) {
    if (strncmp(value, config.server_password, sizeof(config.server_password)) != 0) {
      strncpy(config.server_password, value, sizeof(config.server_password));
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "name") == 0) {
    if (strncmp(value, config.name, sizeof(config.name)) != 0) {
      strncpy(config.name, value, sizeof(config.name));
      config.changed = true;
    }
    return;
  }
  if (strcmp(key, "idle_timeout") == 0) {
    if (value != config.idle_timeout) {
      config.idle_timeout = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "active_timeout") == 0) {
    if (value != config.active_timeout) {
      config.active_timeout = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "adc_interval") == 0) {
    if (value != config.adc_interval) {
      config.adc_interval = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "adc_multiplier") == 0) {
    if (value != config.adc_multiplier) {
      config.adc_multiplier = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "active_threshold") == 0) {
    if (value != config.active_threshold) {
      config.active_threshold = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "card_present_timeout") == 0) {
    if (value != config.card_present_timeout) {
      config.card_present_timeout = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "net_keepalive_interval") == 0) {
    if (value != config.net_keepalive_interval) {
      config.net_keepalive_interval = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "net_retry_min_interval") == 0) {
    if (value != config.net_retry_min_interval) {
      config.net_retry_min_interval = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "net_retry_max_interval") == 0) {
    if (value != config.net_retry_max_interval) {
      config.net_retry_max_interval = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "long_press_time") == 0) {
    if (value != config.long_press_time) {
      config.long_press_time = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "report_status_interval") == 0) {
    if (value != config.report_status_interval) {
      config.report_status_interval= value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "token_query_timeout") == 0) {
    if (value != config.token_query_timeout) {
      config.token_query_timeout = value;
      config.changed = true;
      return;
    }
  }
  if (strcmp(key, "dev") == 0) {
    if (value != config.dev) {
      config.dev = value;
      config.changed = true;
      return;
    }
  }
}

void motd_callback(const char *motd)
{
  display.set_motd(motd);
  display.set_state(device_enabled, device_active);
}

void reboot_callback(bool force)
{
  reboot_pending = true;
  if (force) {
    display.restart_warning();
    ESP.restart();
  }
}

float read_current()
{
  const int valcount = 50;
  const int readings = 4;
  uint16_t values[valcount];
  unsigned int max_val = 0;
  unsigned int min_val = 1024*readings;

  for (int i=0; i<valcount; i++) {
    values[i] = 0;
    for (int j=0; j<readings; j++) {
      values[i] += analogRead(A0);
    }
  }

  for (int i=0; i<valcount; i++) {
    if (values[i] > max_val) {
      max_val = values[i];
    }
    if (values[i] < min_val) {
      min_val = values[i];
    }
  }

  float diff = (float)(max_val - min_val) / 4.0;
  float adc_to_volts = 1.0 / 1024.0;
  float vpp = diff * adc_to_volts;
  float vrms = vpp / 1.414213562 / 2;
  float amps = vrms * 100.0;

  Serial.print("adc_to_volts="); Serial.println(adc_to_volts, 6);
  Serial.print("min="); Serial.println(min_val, DEC);
  Serial.print("max="); Serial.println(max_val, DEC);
  Serial.print("diff="); Serial.println(diff, DEC);
  Serial.print("Vpp="); Serial.println(vpp, 6);
  Serial.print("Vrms="); Serial.println(vrms, 6);
  Serial.print("amps="); Serial.println(amps, 6);

  return ((float)(max_val - min_val) / (float)readings) * config.adc_multiplier / 1000;
  //return (unsigned long)((max_val - min_val) * config.adc_multiplier * 1000) / (config.adc_divider * readings);
}

double read_irms_current()
{
  const int sample_count = 1000;
  static double offsetI = 0;
  double Irms;
  double sumI = 0;

  for (unsigned int n = 0; n < sample_count; n++)
  {
    double sampleI;
    double filteredI;
    double sqI;

    sampleI = analogRead(A0);

    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
    // then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sampleI-offsetI)/1024);
    filteredI = sampleI - offsetI;

    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum
    sumI += sqI;
  }

  Irms = config.adc_multiplier * sqrt(sumI / sample_count) / 1024;
  return Irms;
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

  Serial.begin(115200);
  for (int i=0; i<1024; i++) {
    Serial.print(" \b");
  }
  Serial.println();

  Serial.print(clientid);
  Serial.print(" ");
  Serial.println(ESP.getSketchMD5());

  // run the ADC calculations a few times to stabilise the low-pass filter
  for (int i; i<10; i++) {
    read_irms_current();
    yield();
  }

  Wire.begin(sda_pin, scl_pin);
  display.begin();
  SPIFFS.begin();

  /*
  if (digitalRead(button_a_pin) == LOW || digitalRead(button_b_pin) == LOW) {
    i2c_scan();
    display.message("Entering setup mode");
    Serial.println("button A is down, going into setup mode");
    SetupMode setup_mode(clientid, setup_password);
    setup_mode.run();
  }
  */

  if (!SPIFFS.exists("config.json")) {
    if (SPIFFS.exists("/config.json")) {
      Serial.println("renaming /config.json to config.json");
      SPIFFS.rename("/config.json", "config.json");
    }
  }

  load_config();
  //strcpy(config.ssid, "Hacklab");
  //strcpy(config.wpa_password, "piranhas");
  //strcpy(config.server_host,  "ss-tool-controller.hacklab");
  //config.server_port = 33333;
  //config.changed = true;
  wifisupervisor.begin(config);
  net.begin(config);
  netmsg.begin(config, net, clientid);
  ui.begin();
  display.set_device(config.name);
  display.set_network(network_state_wifi, network_state_ip, network_state_session);

  nfc.token_present_callback = token_present;
  nfc.token_removed_callback = token_removed;
  wifisupervisor.wifi_connected_callback = wifi_connected_callback;
  wifisupervisor.wifi_disconnected_callback = wifi_disconnected_callback;
  netmsg.token_info_callback = token_info_callback;
  netmsg.state_callback = netmsg_state_callback;
  net.state_callback = network_state_callback;
  net.received_json_callback = received_json_callback;
  netmsg.firmware_callback = firmware_callback;
  netmsg.firmware_status_callback = firmware_status_callback;
  netmsg.config_callback = config_callback;
  netmsg.time_callback = time_callback;
  netmsg.file_callback = file_callback;
  netmsg.motd_callback = motd_callback;
  netmsg.reboot_callback = reboot_callback;
  ui.button_callback = button_callback;

  Dir dir = SPIFFS.openDir("");
  while (dir.next()) {
    Serial.print("FILE: ");
    Serial.print(dir.fileName());
    Serial.print(" ");
    Serial.println(dir.fileSize());
  }

  //if (digitalRead(button_a_pin) == LOW && digitalRead(button_b_pin) == LOW) {
  //  while (digitalRead(button_a_pin) == LOW && digitalRead(button_b_pin) == LOW) {
  //    delay(1);
  //  }
  //  SetupMode setup_mode(clientid, setup_password);
  //  setup_mode.run();
  //}

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
    device_milliamps = read_irms_current() * 1000;
    //char t[10];
    //snprintf(t, sizeof(t), "%dmA", device_milliamps);
    //Serial.println(t);
    //display.message(t);
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

void send_file_list()
{
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& files = root.createNestedArray("files");
  root["cmd"] = "file_list_reply";
  Dir dir = SPIFFS.openDir("");
  while (dir.next()) {
    JsonObject& file = files.createNestedObject();
    MD5Builder md5;
    File f = dir.openFile("r");
    md5.begin();
    md5.addStream(f, dir.fileSize());
    md5.calculate();
    file["name"] = dir.fileName();
    file["size"] = dir.fileSize();
    file["md5"] = md5.toString();
  }
  net.send_json(root);
}

void loop() {
  unsigned long loop_start_time;
  unsigned long loop_run_time;
  static bool first_loop = true;
  static unsigned long last_status_send;

  loop_start_time = millis();

  wifisupervisor.loop();
  yield();
  net.loop();
  yield();
  if (config.dev) {
    display.set_attempts(net.myudp.get_attempts());
  }
  if (device_enabled || device_active) {
    display.session_time = session_clock.read();
    display.active_time = active_clock.read();
  }
  display.draw_clocks();
  yield();
  netmsg.loop();
  yield();
  nfc.loop();
  yield();
  display.loop();
  yield();
  buzzer.loop();
  yield();
  ui.loop();
  yield();
  adc_loop();
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

  if (config.changed || first_loop) {
    display.set_device(config.name);
    display.spinner_enabled = false; //config.dev;
    display.current_enabled = config.dev;
    first_loop = false;
  }

  if (config.changed) {
    save_config();
  }

  yield();

  //if (firmware_available && device_enabled == false && device_active == false) {
  //  display.firmware_warning();
  //  Serial.print("we have new firmware available at ");
  //  Serial.println(firmware_url);
  //  Serial.println("attempting update");
  //  delay(1000);
  //  t_httpUpdate_return ret = ESPhttpUpdate.update(firmware_url, firmware_fingerprint);
  //  switch(ret) {
  //    case HTTP_UPDATE_FAILED:
  //      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
  //      break;
  //    case HTTP_UPDATE_NO_UPDATES:
  //      Serial.println("HTTP_UPDATE_NO_UPDATES");
  //      break;
  //    case HTTP_UPDATE_OK:
  //      Serial.println("HTTP_UPDATE_OK");
  //      break;
  //  }
  //  display.refresh();
  //  firmware_available = false;
  //}
  //yield();

  if (status_updated || last_status_send == 0 || millis() - last_status_send > config.report_status_interval) {
    const char *state;
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
    //netmsg.send_status(state, user_name, device_milliamps, active_time, idle_time);
    netmsg.send_status(state, user_name, device_milliamps, active_time, idle_time,
                       net.send_failip_count, net.send_failbegin_count, net.send_failwrite_count, net.send_failend_count);
    last_status_send = millis();
    status_updated = false;
  }

  yield();

  loop_run_time = millis() - loop_start_time;
  if (loop_run_time > 56) {
    Serial.print("loop time ");
    Serial.println(loop_run_time, DEC);
  }

  yield();

  if (firmware_restart_pending) {
    if (device_enabled == false && device_relay == false && device_active == false) {
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

  yield();

  if (reboot_pending) {
    if (device_enabled == false && device_relay == false && device_active == false) {
      Serial.println("rebooting at remote request...");
      net.stop();
      display.restart_warning();
      delay(1000);
      Serial.println("restarting now!");
      ESP.restart();
      delay(5000);
      display.refresh();
    }
  }

  yield();

}
