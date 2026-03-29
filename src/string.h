#pragma once

typedef struct {
  const char *buf;
  unsigned long len;
} string_t;

#define fromconst(str) (string_t){str, sizeof(str) - 1}
