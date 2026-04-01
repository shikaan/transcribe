#pragma once

typedef struct {
  const char *buf;
  unsigned long len;
} string_t;

#define fromconst(str) (string_t){str, sizeof(str) - 1}

typedef enum {
  OK,
  ERROR = -1,

  ERR_AUDIO = -100,
  ERR_AUDIO_INIT = -101,
  ERR_AUDIO_OPENFILE = -102,
  ERR_AUDIO_DECODE = -103
} result_t;
