#pragma once

#include <math.h>
#include <stdbool.h>

static inline float clampf(float x, float min, float max) {
  return fmaxf(min, fminf(x, max));
}

static inline bool overlaps(float rx, float ry, float rw, float rh, float px,
                            float py) {
  return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}
