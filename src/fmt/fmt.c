#include "fmt/fmt.h"
#include "utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

size_t fmt_sectotime(float time, timefmt_t format, char *dest, size_t maxlen) {
  int minutes = (int)time / 60;
  float seconds = time - (float)minutes * 60;

  const char *fmt;
  switch (format) {
  case FMT_TIMECODE:
    fmt = "%02d:%06.3f";
    break;
  case FMT_TIMECODESHORT:
    fmt = "%02d:%02.0f";
    break;
  default:
    assert(false && "fmt_sectotime: unreacheable");
  }

  int len = snprintf(dest, maxlen, fmt, minutes, seconds);
  assert(len > 0 && "fmt_sectotime: cannot write timecode");

  return min(maxlen - 1, (size_t)len);
}

size_t fmt_basename(const char *path, char *dest, size_t maxlen) {
  const char *s = path;
  for (const char *p = path; *p; p++) {
    if (*p == '/' || *p == '\\')
      s = p + 1;
  }
  int len = snprintf(dest, maxlen, "%s", s);
  assert(len > 0 && "fmt_basename: cannot write filename");

  return min(maxlen - 1, (size_t)len);
}

size_t fmt_zoom(float zoom, char *dest, size_t maxlen) { 
  int len = snprintf(dest, maxlen, "%03.1fx", zoom);
  assert(len > 0 && "fmt_zoom: cannot write zoom");

  return min(maxlen - 1, (size_t)len);
}
