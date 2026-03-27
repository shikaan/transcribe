#include "gfx.h"

#include "../lib/sokol_app.h"
#include "../lib/sokol_gfx.h"
#include "../lib/sokol_glue.h"
#include "assets/InterDisplayRegular.16.h"
#include "shader.h"
#include <stdio.h>

typedef struct {
  float buf[MAX_RECTS * VERT_SIZE * VERTS_PER_QUAD];
  int len;
} quads_t;

static quads_t quads;
static color_t bgcol;

static font_t font = {
    .glyphs = (glyph_t *)bakedInterDisplayRegular,
    .nglyphs = sizeof(bakedInterDisplayRegular),
    .atlas = atlasInterDisplayRegular,
    .natlas = sizeof(atlasInterDisplayRegular),
};

static sg_pipeline pipe;
static sg_buffer buf;
static sg_image atlas;
static sg_view atlasview;
static sg_sampler smp;

void logger(const char *tag, uint32_t log_level, uint32_t log_item_id,
            const char *message_or_null, uint32_t line_nr,
            const char *filename_or_null, void *user_data) {
  (void)tag;
  (void)log_level;
  (void)log_item_id;
  (void)user_data;
  printf("%s at %s:%d\n", message_or_null, filename_or_null, line_nr);
}

void gfx_init(void) {
  const sg_desc desc = {.environment = sglue_environment(),
                        .logger.func = logger};
  sg_setup(&desc);

  sg_shader shd = sg_make_shader(shader_shader_desc(sg_query_backend()));

  sg_pipeline_desc pipdesc = {0};
  pipdesc.shader = shd;
  pipdesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
  pipdesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
  pipdesc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4;
  pipdesc.colors[0].blend.enabled = true;
  pipdesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
  pipdesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  pipe = sg_make_pipeline(&pipdesc);

  sg_image_data imagedata = {0};
  sg_range miplvl = {0};
  sg_image_desc imagedesc = {0};
  miplvl.ptr = font.atlas;
  miplvl.size = font.natlas;
  imagedata.mip_levels[0] = miplvl;
  imagedesc.pixel_format = SG_PIXELFORMAT_R8;
  imagedesc.width = 512;
  imagedesc.height = 512;
  imagedesc.data = imagedata;
  atlas = sg_make_image(&imagedesc);

  sg_view_desc viewdesc = {0};
  viewdesc.texture.image = atlas;
  atlasview = sg_make_view(&viewdesc);

  sg_sampler_desc smpdesc = {0};
  smpdesc.min_filter = SG_FILTER_LINEAR;
  smpdesc.mag_filter = SG_FILTER_LINEAR;
  smpdesc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  smpdesc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  smp = sg_make_sampler(&smpdesc);

  sg_buffer_desc bufdesc = {0};
  bufdesc.usage.vertex_buffer = true;
  bufdesc.usage.stream_update = true;
  bufdesc.size = sizeof(quads.buf);
  buf = sg_make_buffer(&bufdesc);
}

void gfx_deinit(void) { sg_shutdown(); }

void gfx_drawstart(color_t color) {
  quads.len = 0;
  bgcol = color;
}

void gfx_drawend(void) {
  int nverts = quads.len * VERTS_PER_QUAD;
  sg_range range = {0};
  range.ptr = quads.buf;
  range.size = (size_t)nverts * VERT_SIZE * sizeof(float);
  sg_update_buffer(buf, &range);

  sg_pass pass = {0};
  pass.swapchain = sglue_swapchain();
  pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.action.colors[0].clear_value =
      (sg_color){bgcol.r, bgcol.g, bgcol.b, bgcol.a};

  sg_bindings bindings = {0};
  bindings.vertex_buffers[0] = buf;
  bindings.views[VIEW_atlas] = atlasview;
  bindings.samplers[SMP_smp] = smp;

  sg_begin_pass(&pass);
  sg_apply_pipeline(pipe);
  sg_apply_bindings(&bindings);
  sg_draw(0, nverts, 1);
  sg_end_pass();
  sg_commit();
}

void gfx_rect(float x, float y, float w, float h, color_t col) {
  float winw = (float)sapp_width();
  float winh = (float)sapp_height();
  float l = (x / winw) * 2.0f - 1.0f;
  float r = ((x + w) / winw) * 2.0f - 1.0f;
  float t = 1.0f - (y / winh) * 2.0f;
  float b = 1.0f - ((y + h) / winh) * 2.0f;

  float quad[] = {
      // clang-format off
    l, t, 1, 1, col.r, col.g, col.b, col.a,
    l, b, 1, 1, col.r, col.g, col.b, col.a,
    r, b, 1, 1, col.r, col.g, col.b, col.a,
    l, t, 1, 1, col.r, col.g, col.b, col.a,
    r, b, 1, 1, col.r, col.g, col.b, col.a,
    r, t, 1, 1, col.r, col.g, col.b, col.a,
      // clang-format on
  };

  int off = quads.len * VERT_SIZE * VERTS_PER_QUAD;
  for (int i = 0; i < VERTS_PER_QUAD * VERT_SIZE; i++)
    quads.buf[off + i] = quad[i];
  quads.len++;
}

void gfx_glyph(float x, float y, char c, color_t col) {
  glyph_t g = font.glyphs[c - 32];

  float winw = (float)sapp_width();
  float winh = (float)sapp_height();
  x = (x / winw) * 2.0f - 1.0f;
  y = 1.0f - (y / winh) * 2.0f;

  float gw = (g.u1 - g.u0) / winw * 2.0f;
  float gh = (g.v1 - g.v0) / winh * 2.0f;
  float xyl = x + g.xoff / winw * 2.0f;
  float xyt = y - g.yoff / winh * 2.0f;
  float xyr = xyl + gw;
  float xyb = xyt - gh;
  float uvl = g.u0 / 512.0f;
  float uvt = g.v0 / 512.0f;
  float uvr = g.u1 / 512.0f;
  float uvb = g.v1 / 512.0f;

  float quad[] = {
      // clang-format off
    xyl, xyt, uvl, uvt, col.r, col.g, col.b, col.a,
    xyl, xyb, uvl, uvb, col.r, col.g, col.b, col.a,
    xyr, xyb, uvr, uvb, col.r, col.g, col.b, col.a,
    xyl, xyt, uvl, uvt, col.r, col.g, col.b, col.a,
    xyr, xyb, uvr, uvb, col.r, col.g, col.b, col.a,
    xyr, xyt, uvr, uvt, col.r, col.g, col.b, col.a,
      // clang-format on
  };

  int off = quads.len * VERT_SIZE * VERTS_PER_QUAD;
  for (int i = 0; i < VERTS_PER_QUAD * VERT_SIZE; i++)
    quads.buf[off + i] = quad[i];
  quads.len++;
}
