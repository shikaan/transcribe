#pragma once

#include <stddef.h>

typedef enum {
  FMT_TIMECODE,
  FMT_TIMECODESHORT,
} timefmt_t;

size_t fmt_sectotime(float time, timefmt_t fmt, char *dest, size_t maxlen);
size_t fmt_basename(const char *path, char *dest, size_t maxlen); 
