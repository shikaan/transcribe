#include "prof.h"

#ifndef NDEBUG
#include <mach/mach_time.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { NSEGMENTS = 32, NLABEL = 16 };

uint64_t times[NSEGMENTS][1 << 20] = {0};
size_t lens[NSEGMENTS] = {0};
char labels[NSEGMENTS][NLABEL] = {0};

static segment_t hash(const char *key) {
  static unsigned long PRIME = 1099511628211U;
  unsigned long hash = 14695981039346656037U;
  const size_t len = strlen(key);

  for (size_t i = 0; i < len; i++) {
    hash ^= (unsigned long)(unsigned char)key[i];
    hash *= PRIME;
  }

  return hash % NSEGMENTS;
}

segment_t prof_segstart(const char *label) {
  segment_t id = hash(label);
  strncpy(labels[id], label, 16);
  times[id][lens[id]] = mach_continuous_time();

  return id;
}

void prof_segend(segment_t id) {
  times[id][lens[id]] = mach_continuous_time() - times[id][lens[id]];
  lens[id]++;
}

void prof_print(void) {
  mach_timebase_info_data_t tb;
  mach_timebase_info(&tb);

  puts("  === Metrics ===");
  for (int id = 0; id < NSEGMENTS; id++) {
    if (lens[id] == 0)
      continue;
    uint64_t total = 0;
    for (unsigned int i = 0; i < lens[id]; i++)
      total += times[id][i];
    uint64_t avg_ns = (total / lens[id]) * tb.numer / tb.denom;
    uint64_t total_ns = total * tb.numer / tb.denom;
    printf("%16s: count=%zu avg=%.3fms total=%.3fms\n", labels[id], lens[id],
           (double)avg_ns / 1e6, (double)total_ns / 1e6);
  }
}
#else

#define prof_segstart(...)
#define prof_segend(...)
#define prof_print(...)

#endif
