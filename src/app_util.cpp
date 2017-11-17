#include "app_util.h"

int decode_hex(const char *hexstr, uint8_t *bytes, size_t max_len)
{
  if (strlen(hexstr) % 2 != 0) {
    return 0;
  }
  int bytelen = strlen(hexstr) / 2;
  if (bytelen > max_len) {
    return 0;
  }
  for (int i=0; i<bytelen; i++) {
    uint8_t ms = hexstr[i*2];
    uint8_t ls = hexstr[(i*2)+1];
    if (ms >= 'a') {
      ms = ms - 'a' + 10;
    } else if (ms >= 'A') {
      ms = ms - 'A' + 10;
    } else {
      ms = ms - '0';
    }
    if (ls >= 'a') {
      ls = ls - 'a' + 10;
    } else if (ls >= 'A') {
      ls = ls - 'A' + 10;
    } else {
      ls = ls - '0';
    }
    bytes[i] = (ms << 4) | ls;
  }
  return bytelen;
}

FileWriter::FileWriter(const char *filename, const char *md5, unsigned int size)
{
  strncpy(_filename, filename, sizeof(_filename));
  strncpy(_md5, md5, sizeof(_md5));
  _size = size;
}

bool FileWriter::up_to_date()
{
  MD5Builder md5;
  File f = SPIFFS.open(_filename, "r");
  md5.begin();
  md5.addStream(f, f.size());
  f.close();
  md5.calculate();
  // TODO: also check size
  Serial.print("file offered md5 local=");
  Serial.print(md5.toString().c_str());
  Serial.print(" remote=");
  Serial.println(_md5);
  if (strcmp(md5.toString().c_str(), _md5) == 0) {
    return true;
  } else {
    return false;
  }
}

bool FileWriter::begin()
{
  file_handle = SPIFFS.open("tmp", "w");
  if (file_handle) {
    md5builder = new MD5Builder();
    md5builder->begin();
    received_size = 0;
    file_open = true;
    Serial.println("begin: file opened");
    return true;
  } else {
    file_open = false;
    Serial.println("begin: file not opened");
    return false;
  }
}

bool FileWriter::add(uint8_t *data, unsigned int len)
{
  if (file_open) {
    md5builder->add(data, len);
    received_size += len;
    return file_handle.write(data, len);
  } else {
    return false;
  }
}

bool FileWriter::add(uint8_t *data, unsigned int len, unsigned int pos)
{
  if (file_open) {
    if (file_handle.seek(pos, SeekSet)) {
      md5builder->add(data, len);
      received_size += len;
      return file_handle.write(data, len);
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool FileWriter::commit()
{
  if (file_handle) {
    file_handle.close();
    file_open = false;
    md5builder->calculate();
    Serial.print("advertised: md5=");
    Serial.print(_md5);
    Serial.print(" size=");
    Serial.println(_size, DEC);
    Serial.print("commit: md5=");
    Serial.print(md5builder->toString());
    Serial.print(" size=");
    Serial.print(received_size, DEC);
    if (_size == received_size && strcmp(md5builder->toString().c_str(), _md5) == 0) {
      Serial.println(" match");
      SPIFFS.remove(_filename);
      SPIFFS.rename("tmp", _filename);
    } else {
      Serial.println(" mismatch!");
      SPIFFS.remove("tmp");
    }
  } else {
    return false;
  }
}

FirmwareWriter::FirmwareWriter(unsigned int size, const char *md5)
{
  _size = size;
  strncpy(_md5, md5, sizeof(_md5));
  position = 0;
  started = 0;
}

bool FirmwareWriter::check()
{
  String current_md5 = ESP.getSketchMD5();
  if (strncmp(current_md5.c_str(), _md5, 32) == 0) {
    return false;
  } else {
    return true;
  }
}

bool FirmwareWriter::begin()
{
  String current_md5 = ESP.getSketchMD5();
  if (strncmp(current_md5.c_str(), _md5, 32) == 0) {
    // this firmware is already installed
    Serial.println("existing firmware has same md5");
    return false;
  }
  if (_size > (unsigned int)ESP.getFreeSketchSpace()) {
    // not enough space for new firmware
    Serial.println("not enough free sketch space");
    return false;
  }
  position = 0;
}

/*
bool FirmwareWriter::add(uint8_t *data, unsigned int len)
{
  if (position == 0) {
    if (len < 4) {
      // need at least 4 bytes to check the file header
      Serial.println("need at least 4 bytes to check the file header");
      return false;
    }
    if (data[0] != 0xE9) {
      // magic header doesn't start with 0xE9
      Serial.println("magic header doesn't start with 0xE9");
      return false;
    }
    uint32_t bin_flash_size = ESP.magicFlashChipSize((data[3] & 0xf0) >> 4);
    // new file doesn't fit into flash
    if (bin_flash_size > ESP.getFlashChipRealSize()) {
      Serial.println("new file won't fit into flash");
      return false;
    }
    if (!Update.begin(_size, U_FLASH)) {
      Serial.println("Update.begin() failed");
      Update.printError(Serial);
      return false;
    }
    if (!Update.setMD5(_md5)) {
      Serial.println("Update.setMD5() failed");
      Update.printError(Serial);
      return false;
    }
    started = true;
  }
  if (!started) {
    return false;
  }
  //Serial.print("FirmwareWriter: writing ");
  //Serial.print(len, DEC);
  //Serial.print(" bytes at position ");
  //Serial.println(position, DEC);
  if (Update.write(data, len) == len) {
    position += len;
    return true;
  } else {
    Update.printError(Serial);
    return false;
  }
}
*/

bool FirmwareWriter::add(uint8_t *data, unsigned int len, unsigned int pos)
{
  if (pos != position) {
    Serial.print("firmware position mismatch (expected=");
    Serial.print(position, DEC);
    Serial.print(" received=");
    Serial.println(pos, DEC);
    return false;
  }
  if (position == 0 && (!started)) {
    if (len < 4) {
      // need at least 4 bytes to check the file header
      Serial.println("need at least 4 bytes to check the file header");
      return false;
    }
    if (data[0] != 0xE9) {
      // magic header doesn't start with 0xE9
      Serial.println("magic header doesn't start with 0xE9");
      return false;
    }
    uint32_t bin_flash_size = ESP.magicFlashChipSize((data[3] & 0xf0) >> 4);
    // new file doesn't fit into flash
    if (bin_flash_size > ESP.getFlashChipRealSize()) {
      Serial.println("new file won't fit into flash");
      return false;
    }
    if (!Update.begin(_size, U_FLASH)) {
      Serial.println("Update.begin() failed");
      Update.printError(Serial);
      return false;
    }
    if (!Update.setMD5(_md5)) {
      Serial.println("Update.setMD5() failed");
      Update.printError(Serial);
      return false;
    }
    started = true;
  }
  if (!started) {
    return false;
  }
  //Serial.print("FirmwareWriter: writing ");
  //Serial.print(len, DEC);
  //Serial.print(" bytes at position ");
  //Serial.println(position, DEC);
  if (Update.write(data, len) == len) {
    position += len;
    return true;
  } else {
    Update.printError(Serial);
    return false;
  }
}

bool FirmwareWriter::commit()
{
  if (started) {
    Serial.println("FirmwareWriter finishing up");
    if (Update.end()) {
      Serial.println("FirmwareWriter: end() succeeded");
      return true;
    } else {
      Serial.println("FirmwareWriter: end() failed");
      Update.printError(Serial);
      Serial.println(Update.getError());
      return false;
    }
  }
}

bool FirmwareWriter::commit_and_reboot()
{
  if (commit()) {
    ESP.restart();
    delay(5000);
  } else {
    return false;
  }
}

void FirmwareWriter::abort()
{

}

unsigned int FirmwareWriter::get_size()
{
  return _size;
}

unsigned int FirmwareWriter::get_position()
{
  return position;
}
