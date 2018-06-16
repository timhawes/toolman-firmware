#ifndef APP_UTIL_H
#define APP_UTIL_H

#include <Arduino.h>
#include <FS.h>

int decode_hex(const char *hexstr, uint8_t *bytes, size_t max_len);

class FileWriter
{
private:
  char _filename[15];
  char _md5[33];
  unsigned int _size;
  bool file_open = false;
  File file_handle;
  MD5Builder *md5builder;
  unsigned int received_size;
public:
  FileWriter(const char *filename, const char *md5, unsigned int size);
  bool up_to_date();
  bool begin();
  bool add(uint8_t *data, unsigned int len);
  bool add(uint8_t *data, unsigned int len, unsigned int pos);
  bool commit();
};

class FirmwareWriter
{
private:
  unsigned int _size;
  char _md5[33];
  bool file_open = false;
  MD5Builder *md5builder;
  unsigned int position;
  bool started;
public:
  FirmwareWriter(unsigned int size, const char *md5);
  bool check();
  bool begin();
  //bool add(uint8_t *data, unsigned int len);
  bool add(uint8_t *data, unsigned int len, unsigned int pos);
  bool commit();
  bool commit_and_reboot();
  void abort();
  unsigned int get_size();
  unsigned int get_position();
};

class MilliClock
{
private:
  unsigned long start_time;
  unsigned long running_time;
  bool running;
public:
  MilliClock();
  unsigned long read();
  void start();
  void stop();
  void reset();
};

#endif
