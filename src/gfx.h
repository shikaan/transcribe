#pragma once

enum {
  MAX_RECTS = 1024,
  VERT_SIZE = 8,
  VERTS_PER_QUAD = 6, // double triangle
};

// Color in RGBA, each channel in [0.0, 1.0]
typedef struct {
  float r, g, b, a;
} color_t;

typedef struct {
  unsigned short u0, v0, u1, v1;
  float xoff, yoff, xadvance;
} glyph_t;

typedef struct {
  glyph_t *glyphs;
  unsigned char *atlas;
  unsigned int nglyphs;
  unsigned int natlas;
} font_t;

void gfx_init(void);
void gfx_deinit(void);
void gfx_drawstart(color_t bg);
void gfx_drawend(void);

void gfx_rect(float x, float y, float w, float h, color_t col);
void gfx_glyph(float x, float y, char a, color_t col);
