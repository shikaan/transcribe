#pragma once

#include "../types.h"

enum {
  MAX_QUADS = 1024,
  VERT_SIZE = 8,
  VERTS_PER_QUAD = 6,
};

typedef enum {
  NO_FONT = -1,

  FONT_SANS_BOLD_S,
  FONT_SANS_M,
  FONT_SANS_BOLD_M,
  FONT_ICONS_M,
  FONT_ICONS_L,
  FONT_ICONS_XL,
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
  float asc;
  float desc;
} font_t;

typedef struct {
  fonttype_t font;
  float hspacing;
} textdesc_t;

typedef struct {
  // Width of the string as drawn (sum of glyph advances).
  float w;
  // Tight height of this string's glyphs. Varies per string.
  // Use asc/desc for string-independent vertical metrics.
  float h;
  // Font's max ascent above baseline. Constant per font.
  float asc;
  // Font's max descent below baseline. Constant per font.
  float desc;
} textmeasure_t;

typedef enum {
  LINEDIR_H,
  LINEDIR_V,
} linedir_t;

void gfx_init(void);
void gfx_deinit(void);
void gfx_drawstart(color_t color);
void gfx_drawend(void);

void gfx_rect(float x, float y, float w, float h, color_t col);
void gfx_text(float x, float y, string_t string, textdesc_t desc, color_t col);
void gfx_measuretext(string_t string, textdesc_t desc, textmeasure_t *measure);

// Color Utils
color_t gfx_darken(color_t c, float amount);
color_t gfx_lighten(color_t c, float amount);
color_t gfx_setalpha(color_t c, float amount);

// Higher-level functions
void gfx_line(float x, float y, float len, linedir_t dir, color_t col);
