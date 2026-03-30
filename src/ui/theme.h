#pragma once

#include "gfx/gfx.h"

#define color(r, g, b, a)                                                      \
  {(float)r / 0xff, (float)g / 0xff, (float)b / 0xff, (float)a / 0xff}

enum {
  COLOR_BG,
  COLOR_TEXTHIGHLIGHT,
  COLOR_BARBG,
  COLOR_TEXTDIM,
  COLOR_TEXTLIGHT,
  COLOR_PRIMARY,
  COLOR_SPECTRUMBG,

  COLORS,
};

typedef struct {
  color_t colors[COLORS];
  float spacing;
} theme_t;

static theme_t dark = {
    .colors =
        {
            color(0x3d, 0x42, 0x4c, 0xff), // COLOR_BG
            color(0xdd, 0xdf, 0xe3, 0xff), // COLOR_FG
            color(0x11, 0x13, 0x17, 0xff), // COLOR_BARBG
            color(0x3d, 0x42, 0x4c, 0xff), // COLOR_TEXTDIM
            color(0x90, 0x96, 0xa1, 0xff), // COLOR_TEXTLIGHT
            color(0xf3, 0xa0, 0x24, 0xff), // COLOR_PRIMARY
            color(0x19, 0x1b, 0x20, 0xff), // COLOR_SPECTRUMBG
        },
    .spacing = 8,
};

static theme_t *theme = &dark;
