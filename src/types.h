#pragma once

typedef struct {
  const char *buf;
  unsigned long len;
} string_t;

#define string(str) (string_t){str, sizeof(str) - 1}

typedef enum {
  OK,
  ERROR = -1,

  ERR_AUDIO = -100,
  ERR_AUDIO_INIT = -101,
  ERR_AUDIO_OPENFILE = -102,
  ERR_AUDIO_DECODE = -103,
  ERR_AUDIO_ALLOC = -104,

  ERR_FILES = -200,
  ERR_FILES_CANCEL = -201
} result_t;
