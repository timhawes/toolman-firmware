#include "ArduinoConfigDB.hpp"
#include <FS.h>

ArduinoConfigDB::ArduinoConfigDB()
{

}

bool ArduinoConfigDB::load()
{
  if (SPIFFS.exists(filename)) {
    File configFile = SPIFFS.open(filename, "r");
    if (configFile) {
      while (configFile.available()) {
        String k = configFile.readStringUntil(0);
        String v = configFile.readStringUntil(0);
        data[k] = v;
      }
      configFile.close();
      changed = false;
      Serial.println("ArduinoConfigDB: loaded");
      return true;
    } else {
      Serial.println("ArduinoConfigDB: error loading file");
      return false;
    }
  } else {
    Serial.println("ArduinoConfigDB: file not found");
    return true;
  }
}

bool ArduinoConfigDB::save(bool force)
{
  if (force || changed) {
    int length = 0;
    int written = 0;
    File configFile = SPIFFS.open(tmp_filename, "w");
    if (configFile) {
      for (auto kv : data) {
        length += kv.first.length() + kv.second.length() + 2;
        written += configFile.print(kv.first.c_str());
        written += configFile.write(0);
        written += configFile.print(kv.second.c_str());
        written += configFile.write(0);
      }
      configFile.close();
      if (length == written) {
        Serial.println("ArduinoConfigDB: saved");
        SPIFFS.remove(filename);
        SPIFFS.rename(tmp_filename, filename);
        changed = false;
        return true;
      }
    }
    Serial.println("ArduinoConfigDB: error saving");
    SPIFFS.remove(tmp_filename);
    // even though the save failed, we'll assume that the changes have been committed
    // otherwise we would keep trying to save and damage the flash
    changed = false;
    return false;
  } else {
    Serial.println("ArduinoConfigDB: no changes");
    return true;
  }
}

void ArduinoConfigDB::dump()
{
  for (auto kv : data) {
    Serial.print("CONFIG ");
    Serial.print(kv.first.c_str());
    Serial.print(" = ");
    Serial.println(kv.second.c_str());
  }
}

void ArduinoConfigDB::clear()
{
  data.clear();
  changed = true;
}

String ArduinoConfigDB::getString(const char *key)
{
  return data[key];
}

const char* ArduinoConfigDB::getConstChar(const char *key)
{
  return data[key].c_str();
}

long ArduinoConfigDB::getInteger(const char *key)
{
  return data[key].toInt();
}

bool ArduinoConfigDB::getBoolean(const char *key)
{
  if (data[key].equals("true")) {
    return true;
  } else if (data[key].equals("false")) {
    return false;
  } else {
    return (bool)(data[key].toInt());
  }
}

void ArduinoConfigDB::set(const char *key, String value)
{
  if (!value.equals(data[key])) {
    Serial.print("CONFIG ");
    Serial.print(key);
    Serial.print(" ");
    Serial.print(data[key]);
    Serial.print(" -> ");
    Serial.println(value);
    data[key] = value;
    changed = true;
  }
}

void ArduinoConfigDB::set(const char *key, const char *value)
{
  set(key, String(value));
}

void ArduinoConfigDB::set(const char *key, long value)
{
  set(key, String(value, DEC));
}

void ArduinoConfigDB::set(const char *key, int value)
{
  set(key, String(value, DEC));
}

void ArduinoConfigDB::set(const char *key, bool value)
{
  if (value) {
    set(key, "1");
  } else {
    set(key, "0");
  }
}

void ArduinoConfigDB::remove(const char *key)
{
  if (data.count(key) != 0) {
    Serial.print("CONFIG ");
    Serial.print(key);
    Serial.print(" ");
    Serial.print(data[key]);
    Serial.println(" -> [removed]");
    data.erase(key);
    changed = true;
  }
}

bool ArduinoConfigDB::hasChanged()
{
  return changed;
}
