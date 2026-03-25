#pragma once

#include "../lib/sokol_app.h"
#include "../lib/sokol_gfx.h"
#include "../lib/sokol_glue.h"
#include "shader.h"
#include <stdio.h>

enum {
  MAX_RECTS = 1024,
  VERT_SIZE = 6,
  VERTS_PER_RECT = 6, // double triangle
};

typedef struct {
  float buf[MAX_RECTS * VERT_SIZE * VERTS_PER_RECT];
  int len;
} rects_t;

// Color in RGBA
typedef sg_color color_t;

// Vertices in the CPU that will be transferred to the GPU for rendering.
static rects_t rects;
// GPU pipeline for rectangles. Currently the only one.
static sg_pipeline pip;
// Buffer to be managed by the GPU.
static sg_buffer buf;

static color_t bgcol;

static void gfx_init(void) {
  const sg_desc desc = {.environment = sglue_environment()};
  sg_setup(&desc);

  sg_shader shd = sg_make_shader(shader_shader_desc(sg_query_backend()));

  sg_pipeline_desc pipdesc = {0};
  pipdesc.shader = shd;
  pipdesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
  pipdesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT4;
  pipdesc.colors[0].blend.enabled = true;
  pipdesc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA;
  pipdesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  pip = sg_make_pipeline(&pipdesc);

  sg_buffer_desc bufdesc = {0};
  bufdesc.usage.vertex_buffer = true;
  bufdesc.usage.stream_update = true;
  bufdesc.size = sizeof(rects.buf);

  buf = sg_make_buffer(&bufdesc);
}

static void gfx_deinit(void) { sg_shutdown(); }

static void gfx_drawstart(color_t color) {
  rects.len = 0;
  bgcol = color;
}

static void gfx_drawend(void) {
  int nverts = rects.len * VERTS_PER_RECT;
  sg_range range = {0};
  range.ptr = rects.buf;
  range.size = (size_t)nverts * VERT_SIZE * sizeof(float);
  sg_update_buffer(buf, &range);

  sg_pass pass = {0};
  pass.swapchain = sglue_swapchain();
  pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.action.colors[0].clear_value = bgcol;

  sg_bindings bindings = {0};
  bindings.vertex_buffers[0] = buf;

  sg_begin_pass(&pass);
  sg_apply_pipeline(pip);
  sg_apply_bindings(&bindings);
  sg_draw(0, nverts, 1);
  sg_end_pass();
  sg_commit();
}

static void gfx_rect(float x, float y, float w, float h, color_t col) {
  float winw = (float)sapp_width();
  float winh = (float)sapp_height();
  float l = (x / winw) * 2.0f - 1.0f;
  float ri = ((x + w) / winw) * 2.0f - 1.0f;
  float t = 1.0f - (y / winh) * 2.0f;
  float bo = 1.0f - ((y + h) / winh) * 2.0f;

  float quad[] = {
      // clang-format off
      l,   t, col.r, col.g, col.b, col.a,
      l,  bo, col.r, col.g, col.b, col.a,
      ri, bo, col.r, col.g, col.b, col.a,
      l,   t, col.r, col.g, col.b, col.a,
      ri, bo, col.r, col.g, col.b, col.a,
      ri,  t, col.r, col.g, col.b, col.a,
      // clang-format on
  };

  int off = rects.len * VERT_SIZE * VERTS_PER_RECT;
  for (int i = 0; i < VERTS_PER_RECT * VERT_SIZE; i++)
    rects.buf[off + i] = quad[i];
  rects.len++;
}
