#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
  FMT_TIMECODE,
  FMT_TIMECODESHORT,
} timefmt_t;

int fmt_sectotime(float time, char *buffer, timefmt_t fmt) {
  int minutes = (int)time / 60;
  float seconds = time - (float)minutes * 60;

  switch (fmt) {
  case FMT_TIMECODE:
    return snprintf(buffer, 10, "%02d:%06.3f", minutes, seconds);
  case FMT_TIMECODESHORT:
    return snprintf(buffer, 6, "%02d:%02.0f", minutes, seconds);
  default:
    assert(false && "unreacheable");
  }
}
