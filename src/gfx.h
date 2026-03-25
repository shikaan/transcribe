#pragma once

enum {
  MAX_RECTS = 1024,
  VERT_SIZE = 6,
  VERTS_PER_RECT = 6, // double triangle
};

// Color in RGBA, each channel in [0.0, 1.0]
typedef struct {
  float r, g, b, a;
} color_t;

void gfx_init(void);
void gfx_deinit(void);
void gfx_drawstart(color_t color);
void gfx_drawend(void);
void gfx_rect(float x, float y, float w, float h, color_t col);
