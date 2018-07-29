#include "app_netmsg.h"
#include "base64.hpp"

NetMsg::NetMsg()
{
}

void NetMsg::begin(ArduinoConfigDB &config, Network &network, const char *clientid)
{
  _config = &config;
  _network = &network;
  _clientid = clientid;
  last_send = millis();
  last_hello = 0;
}

void NetMsg::loop()
{
  if (millis() - last_send > _config->getInteger("net_keepalive_interval")) {
    // send keepalive if we haven't transmitted in last X seconds
    send_keepalive();
  }
  /* else if ((millis() - last_receive > _config->net_keepalive_interval) && (millis() - last_send > 5000)) {
    // send keepalive every 5 seconds if we haven't received in the last 30 seconds
    send_keepalive();
  }*/

  if (strlen(pending_token) && millis() - last_token_query > _config->getInteger("token_query_timeout")) {
    Serial.println("token query timed-out");
    if (token_info_callback) {
      token_info_callback(pending_token, false, "", 0);
    }
    pending_token[0] = 0;
  }

}

bool NetMsg::send_json(JsonObject &data)
{
  last_send = millis();
  return _network->send_json(data);
}

void NetMsg::send_keepalive()
{
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "ping";
  root["millis"] = millis();
  send_json(root);
}

/*
void NetMsg::token(const char *uid)
{
  StaticJsonBuffer<100> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "token-auth";
  root["uid"] = uid;
  strncpy(pending_token, uid, sizeof(pending_token));
  last_token_query = millis();
  send_json(root);
}
*/

void NetMsg::token(NFCToken token)
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "token-auth";
  root["uid"] = token.uidString();
  if (token.ats_len > 0) {
    root["ats"] = hexlify(token.ats, token.ats_len);
  }
  root["atqa"] = (int)token.atqa;
  root["sak"] = (int)token.sak;
  if (token.version_len > 0) {
    root["version"] = hexlify(token.version, token.version_len);
  }
  if (token.ntag_counter > 0) {
    root["ntag_counter"] = (long)token.ntag_counter;
  }
  if (token.ntag_signature_len > 0) {
    root["ntag_signature"] = hexlify(token.ntag_signature, token.ntag_signature_len);
  }
  strncpy(pending_token, token.uidString().c_str(), sizeof(pending_token));
  last_token_query = millis();
  send_json(root);
}

bool NetMsg::get_file(const char *filename)
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "file-request";
  root["filename"] = filename;
  root["position"] = 0;
  return send_json(root);
}

bool NetMsg::receive_json(char *data)
{
  //Serial.print("recv> ");
  //Serial.println(data);
  int inputlen = strlen(data);
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  if (root.success()) {
    last_receive = millis();
    if (jsonBuffer.size() > max_received_jsonbuffer) {
      max_received_jsonbuffer = jsonBuffer.size();
      Serial.print("jsonBuffer ");
      Serial.print(max_received_jsonbuffer, DEC);
      Serial.print(" for input of ");
      Serial.println(inputlen, DEC);
    }
    String cmd = root["cmd"];
    if (cmd == "hello") {
      data_cmd_hello(root);
    } else if (cmd == "bye") {
      data_cmd_bye(root);
    } else if (cmd == "auth-ok") {
      data_cmd_auth_ok(root);
    } else if (cmd == "auth_ok") {
      data_cmd_auth_ok(root);
    } else if (cmd == "config") {
      data_cmd_config(root);
    } else if (cmd == "token-info") {
      data_cmd_token_info(root);
    } else if (cmd == "token_info") {
      data_cmd_token_info(root);
    } else if (cmd == "database-info") {
      data_cmd_database_info(root);
    } else if (cmd == "database_info") {
      data_cmd_database_info(root);
    } else if (cmd == "ping") {
      //data_cmd_ping(root);
    } else if (cmd == "pong") {
      //data_cmd_pong(root);
    } else if (cmd == "file_list") {
      data_cmd_file_list(root);
    } else if (cmd == "file") {
      data_cmd_file(root);
    } else if (cmd == "file_payload") {
      data_cmd_file_payload(root);
    } else if (cmd == "firmware") {
      data_cmd_firmware(root);
    } else if (cmd == "firmware_payload") {
      data_cmd_firmware_payload(root);
    } else if (cmd == "motd") {
      data_cmd_motd(root);
    } else if (cmd == "reboot") {
      data_cmd_reboot(root);
    } else {
      if (!root.containsKey("ack")) {
        Serial.println("Unknown command from server");
      }
    }
  } else {
    Serial.print("parseObject() failed");
    return true;
  }
}

void NetMsg::network_changed()
{
  if (state_callback) {
    state_callback(false, "");
  }
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "hello";
  _network->send_json(root);
}

void NetMsg::data_cmd_hello(JsonObject &data)
{
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "auth";
  root["client"] = _clientid;
  root["password"] = _config->getString("server_password");
  _network->flush();
  _network->send_json(root);
}

void NetMsg::data_cmd_bye(JsonObject &data)
{
  connected = false;
  if (state_callback) {
    state_callback(false, "");
  }
}

void NetMsg::data_cmd_auth_ok(JsonObject &data)
{
  connected = true;
  if (state_callback) {
    state_callback(true, "");
  }
  send_esp_data();
}

void NetMsg::data_cmd_config(JsonObject &data)
{
  if (config_callback) {
    for (auto kv : data) {
      if (strcmp(kv.key, "cmd") == 0) {
        // ignore cmd key
      } else {
        config_callback(kv.key, kv.value.as<char*>());
      }
    }
  }
}

void NetMsg::data_cmd_ping(JsonObject &data)
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "pong";
  send_json(root);
}

void NetMsg::data_cmd_token_info(JsonObject &data)
{
  //StaticJsonBuffer<256> jsonBuffer;
  //JsonObject& root = jsonBuffer.createObject();
  //root["cmd"] = "debug";
  //root["pending"] = (const char*)pending_token;
  //root["received"] = data["uid"];
  //send_json(root);
  if (strlen(pending_token) != 0 && strcmp(pending_token, (const char*)data["uid"]) == 0) {
    pending_token[0] = 0;
    if (token_info_callback) {
      token_info_callback(data["uid"], data["found"], data["name"], data["access"]);
    }
  } else {
    Serial.println("received token info that doesn't match the pending request");
  }
}

void NetMsg::data_cmd_motd(JsonObject &data)
{
  if (motd_callback) {
    motd_callback(data["motd"]);
  }
}

void NetMsg::data_cmd_database_info(JsonObject &data)
{
  if (database_callback) {
  }
}

void NetMsg::data_cmd_firmware(JsonObject &data)
{
  Serial.println("server has offered a firmware");
  FirmwareWriter *f = new FirmwareWriter(data["size"], data["md5"]);
  Serial.println("check if we want to keep it...");
  if (f->check()) {
    delete f;
    Serial.println("new firmware available");
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cmd"] = "firmware_request";
    root["position"] = 0;
    root["chunk_size"] = data_chunk_size;
    root["md5"] = data["md5"];
    send_json(root);
  } else {
    Serial.println("nope, don't bother");
    delete f;
  }
}

void NetMsg::data_cmd_firmware_payload(JsonObject &data)
{
  const char *b64 = data.get<const char*>("data");
  unsigned int max_length = (strlen(b64) * 3 / 4);
  //Serial.print("b64 len ");
  //Serial.println(strlen(b64), DEC);
  //Serial.print("allocated buffer ");
  //Serial.println(max_length, DEC);
  uint8_t binary[max_length];
  unsigned int binary_length;

  binary_length = decode_base64((unsigned char*)b64, binary);
  //Serial.print("binary len ");
  //Serial.println(binary_length, DEC);

  if (data["start"] && (!firmwarewriter)) {
    Serial.println("new FirmwareWriter");
    if (firmwarewriter) {
      Serial.println("FirmwareWriter is already active, restarting it");
      firmwarewriter->abort();
      delete firmwarewriter;
      firmwarewriter = NULL;
    }
    firmwarewriter = new FirmwareWriter(data["size"], data["md5"].as<String>().c_str());
    Serial.println("firmwriter->begin");
    firmwarewriter->begin();
  }

  if (data["position"] != firmwarewriter->get_position()) {
    // TODO: this should be removable when we correctly ack data in both directions
    Serial.println("incoming packet not at correct position");
    //StaticJsonBuffer<256> jsonBuffer;
    //JsonObject& root = jsonBuffer.createObject();
    //root["cmd"] = "firmware_request";
    //root["position"] = (unsigned int)firmwarewriter->get_position();
    //root["chunk_size"] = 512;
    //send_json(root);
    return;
  }

  //firmwarewriter->add(binary, binary_length);
  if (!firmwarewriter->add(binary, binary_length, data["position"])) {
    Serial.println("firmwarewriter->add failed");
    return;
  }

  if (firmware_status_callback) {
    firmware_status_callback(true, false, 100*data["position"].as<unsigned int>()/firmwarewriter->get_size());
  }

  if (data["end"]) {
    Serial.println("firmwarewriter->commit()");
    if (firmwarewriter->commit()) {
      Serial.println("firmwarewriter->commit() done");
      if (firmware_status_callback) {
        firmware_status_callback(false, true, 100);
      }
    } else {
      Serial.println("firmwarewriter->commit() failed");
      if (firmware_status_callback) {
        firmware_status_callback(false, false, 100);
      }
    }
    delete firmwarewriter;
    firmwarewriter = NULL;
  } else {
    //Serial.println("asking for next block");
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cmd"] = "firmware_request";
    root["position"] = (unsigned int)data["position"] + binary_length;
    root["chunk_size"] = data_chunk_size;
    //Serial.println("asking for next block... send_json...");
    send_json(root);
    //Serial.println("asking for next block... send_json returned");
  }

  return;
}

void NetMsg::send_esp_data()
{
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "esp_data";
  root["free_heap"] = ESP.getFreeHeap();
  root["chip_id"] = ESP.getChipId();
  root["sdk_version"] = ESP.getSdkVersion();
  root["core_version"] = ESP.getCoreVersion();
  root["boot_version"] = ESP.getBootVersion();
  root["boot_mode"] = ESP.getBootMode();
  root["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
  root["flash_chip_id"] = ESP.getFlashChipId();
  root["flash_chip_real_size"] = ESP.getFlashChipRealSize();
  root["flash_chip_size"] = ESP.getFlashChipSize();
  root["flash_chip_speed"] = ESP.getFlashChipSpeed();
  root["flash_chip_mode"] = ESP.getFlashChipMode();
  root["flash_chip_size_by_chip_id"] = ESP.getFlashChipSizeByChipId();
  root["sketch_size"] = ESP.getSketchSize();
  root["sketch_md5"] = ESP.getSketchMD5();
  root["free_sketch_space"] = ESP.getFreeSketchSpace();
  root["reset_reason"] = ESP.getResetReason();
  root["reset_info"] = ESP.getResetInfo();
  root["cycle_count"] = ESP.getCycleCount();
  root["millis"] = millis();
  send_json(root);
}

void NetMsg::data_cmd_file_list(JsonObject &data)
{
/*  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "file_list_reply";
  JsonArray& files = root.createNestedArray("files");
  Dir dir = SPIFFS.openDir("");
  while (dir.next()) {
    files.add(dir.fileName());
    Serial.print(dir.fileName());
    Serial.print(" ");
    Serial.println(dir.fileSize());
  }
  _network->send_json(root);
*/
}

void NetMsg::data_cmd_file(JsonObject &data)
{
  FileWriter *f = new FileWriter(data["filename"], data["md5"], data["size"]);
  if (f->up_to_date()) {
    delete f;
    Serial.print("received offer of file ");
    Serial.print((const char*)data["filename"]);
    Serial.println(" but already have that md5");
    return;
  } else {
    delete f;
    Serial.print("received offer of file ");
    Serial.print((const char*)data["filename"]);
    Serial.println(", asking for data");
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cmd"] = "file_request";
    root["filename"] = data["filename"];
    root["position"] = 0;
    root["chunk_size"] = 256;
    send_json(root);
  }
}

void NetMsg::data_cmd_file_payload(JsonObject &data)
{
  const char *b64 = data.get<const char*>("data");
  uint8_t binary[300];
  unsigned int binary_length;

  binary_length = decode_base64((unsigned char*)b64, binary);

  if (data["start"]) {
    Serial.println("new FileWriter");
    filewriter = new FileWriter(data["filename"].as<String>().c_str(), data["md5"].as<String>().c_str(), data["size"]);
    Serial.println("filewriter->begin");
    filewriter->begin();
  }

  Serial.print("writing file at position ");
  Serial.println((unsigned int)data["position"], DEC);

  if (!filewriter->add(binary, binary_length, data["position"])) {
    Serial.println("firmware load aborted");
    firmwarewriter->abort();
    delete firmwarewriter;
    firmwarewriter = NULL;
    return;
  }
  //filewriter->add(binary, binary_length);

  if (data["end"]) {
    Serial.println("filewriter->commit()");
    bool result = filewriter->commit();
    Serial.println("filewriter->commit() done");
    delete filewriter;
    filewriter = NULL;
  } else {
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["cmd"] = "file_request";
    root["filename"] = data["filename"];
    root["position"] = (unsigned int)data["position"] + binary_length;
    root["chunk_size"] = 256;
    send_json(root);
  }

  return;
}

void NetMsg::data_cmd_reboot(ArduinoJson::JsonObject &data)
{
  if (reboot_callback) {
    if (data["force"]) {
      reboot_callback(true);
    } else {
      reboot_callback(false);
    }
  }
}

void NetMsg::send_status(const char *status, const char *user, unsigned long milliamps, long active_time, long idle_time)
{
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "status";
  root["status"] = status;
  root["user"] = user;
  root["milliamps"] = milliamps;
  root["active_time"] = active_time;
  root["idle_time"] = idle_time;
  root["millis"] = millis();
  send_json(root);
}

void NetMsg::send_status(const char *status, const char *user, unsigned long milliamps, long active_time, long idle_time, unsigned long send_failip_count, unsigned long send_failbegin_count, unsigned long send_failwrite_count, unsigned long send_failend_count, unsigned long udp_retry_count)
{
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cmd"] = "status";
  root["status"] = status;
  root["user"] = user;
  root["milliamps"] = milliamps;
  root["active_time"] = active_time;
  root["idle_time"] = idle_time;
  root["send_failip_count"] = send_failip_count;
  root["send_failbegin_count"] = send_failbegin_count;
  root["send_failwrite_count"] = send_failwrite_count;
  root["send_failend_count"] = send_failend_count;
  root["udp_retry_count"] = udp_retry_count;
  root["millis"] = millis();
  send_json(root);
}
