#pragma once

#include "gfx.h"

#define color(r, g, b, a)                                                      \
  {(float)r / 0xff, (float)g / 0xff, (float)b / 0xff, (float)a / 0xff}

enum {
  COLOR_BACKGROUND,
  COLOR_BARBACKGROUND,
  COLOR_FOREGROUND,
  COLOR_TEXTDIM,
  COLOR_TEXTLIGHT,
  COLORS,
};

typedef struct {
  color_t colors[COLORS];
  float spacing;
  // font_t fonts[FONTS];
} theme_t;

static theme_t dark = {
    .colors =
        {
            color(0x3d, 0x42, 0x4c, 0xff), // COLOR_BACKGROUND
            color(0x11, 0x13, 0x17, 0xff), // COLOR_BARBACKGROUND
            color(0xdd, 0xdf, 0xe3, 0xff), // COLOR_FOREGROUND
            color(0x3d, 0x42, 0x4c, 0xff), // COLOR_TEXTDIM
            color(0x90, 0x96, 0xa1, 0xff), // COLOR_TEXTLIGHT
        },
    .spacing = 8,
};

static theme_t *theme = &dark;
