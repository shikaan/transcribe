#pragma once

#include <stdbool.h>
#include <assert.h>

#define min(A, B) ((A) > (B) ? (B) : (A))

#define max(A, B) ((A) < (B) ? (B) : (A))

#define clamp(N, Min, Max) max((Min), min((N), (Max))) 

static inline bool overlaps(float rx, float ry, float rw, float rh, float px,
                            float py) {
  return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

#define len(Arr) (sizeof(Arr)/sizeof(Arr[0]))
