#pragma once

#include "string.h"

enum {
  MAX_QUADS = 1024,
  VERT_SIZE = 8,
  VERTS_PER_QUAD = 6,
};

typedef enum {
  FONT_SANS_M,
  FONT_SANS_BOLD_M,
  FONT_ICONS_M,
  FONT_MONO_S,
  FONT_MONO_M,
  FONT_MONO_L,

  FONTS
} fonttype_t;

// Color in RGBA, each channel in [0.0, 1.0]
typedef struct {
  float r, g, b, a;
} color_t;

typedef struct {
  unsigned short x0, y0, x1, y1;
  float xoff, yoff, xadvance;
} glyph_t;

typedef struct {
  glyph_t *glyphs;
  unsigned char *bitmap;
  unsigned int nglyphs;
  unsigned int nbitmap;
} font_t;

void gfx_init(void);
void gfx_deinit(void);
void gfx_drawstart(color_t bg);
void gfx_drawend(void);

void gfx_rect(float x, float y, float w, float h, color_t col);
void gfx_text(float x, float y, string_t string, fonttype_t font, color_t col);
void gfx_textbox(string_t string, fonttype_t font, float *w, float *h);
