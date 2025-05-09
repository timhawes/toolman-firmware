// SPDX-FileCopyrightText: 2018-2024 Tim Hawes
//
// SPDX-License-Identifier: MIT

#ifndef TOKENDB_HPP
#define TOKENDB_HPP

#include <Arduino.h>
#include <FS.h>

class TokenDB
{
private:
  const char *_filename;
  int access_level;
  int dbversion = -1;
  String user;
  bool query_v1(File file, uint8_t uidlen, uint8_t *uid);
  bool query_v2(File file, uint8_t uidlen, uint8_t *uid);
  bool query_v3(File file, uint8_t uidlen, uint8_t *uid);
public:
  TokenDB(const char *filename);
  bool lookup(uint8_t uidlen, uint8_t *uid);
  bool lookup(const char *uid);
  int get_access_level();
  int get_version();
  String get_user();
};

#endif
