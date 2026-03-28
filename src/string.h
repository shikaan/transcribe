#pragma once

typedef struct {
  const char *buf;
  unsigned long len;
} string_t;

#define fromconst(str) {str, sizeof(str) - 1}
