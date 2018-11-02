#ifndef ARDUINOCONFIGDB_HPP
#define ARDUINOCONFIGDB_HPP

#include <map>
#include <Arduino.h>

class ArduinoConfigDB
{
private:
  const char *filename = "config.dat";
  const char *tmp_filename = "config.tmp";
  std::map<String, String> data;
  bool changed;
public:
  ArduinoConfigDB();
  bool load();
  bool save(bool force=false);
  void dump();
  void clear();
  std::map<String, String> &getMap();
  String getString(const char *key);
  const char* getConstChar(const char *key);
  long getInteger(const char *key);
  bool getBoolean(const char *key);
  void set(const char *key, String value);
  void set(const char *key, const char *value);
  void set(const char *key, long value);
  void set(const char *key, int value);
  void set(const char *key, bool value);
  void remove(const char *key);
  bool hasChanged();
};

#endif
