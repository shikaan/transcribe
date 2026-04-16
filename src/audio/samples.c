#include "audio/audio.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Allocates or reuses a samples buffer of `len` floats. Reuses the existing
// allocation if capacity is sufficient; zeroes the content on reuse.
void *samples_alloc(unsigned int len, samples_t *s) {
  assert(len > 0 && "samples_alloc: len must be > 0");
  if (s->buf) {
    if (s->cap >= len) {
      memset(s->buf, 0, s->len * sizeof(float));
      s->len = len;
    } else {
      float *buf = realloc(s->buf, len * sizeof(float));
      if (!buf) {
        // TODO: ERROR HANDLING
        return s->buf;
      }
      s->buf = buf;
      s->len = len;
      s->cap = len;
    }
  } else {
    s->buf = malloc(len * sizeof(float));
    s->len = len;
    s->cap = len;
  }
  return s->buf;
}

