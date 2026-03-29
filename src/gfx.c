#include "gfx.h"

#include "../lib/sokol_app.h"
#include "../lib/sokol_gfx.h"
#include "../lib/sokol_glue.h"
#include "assets/Icons.14@2x.h"
#include "assets/Icons.20@2x.h"
#include "assets/Icons.32@2x.h"
#include "assets/InterDisplayRegular.12@2x.h"
#include "assets/InterDisplayRegular.14@2x.h"
#include "assets/InterDisplaySemiBold.14@2x.h"
#include "assets/JetBrainsMonoRegular.12@2x.h"
#include "assets/JetBrainsMonoRegular.14@2x.h"
#include "assets/JetBrainsMonoRegular.20@2x.h"
#include "shader.h"
#include "string.h"
#include <stddef.h>
#include <stdio.h>

enum {
  QUADS = FONTS + 1, // a buffer per font and one for rects
  RECT = FONTS,      // last quad buffer is for rects
};

typedef struct {
  float buf[MAX_QUADS * VERT_SIZE * VERTS_PER_QUAD];
  size_t len;
} quads_t;

static quads_t quads[QUADS];
static sg_buffer buf[QUADS];

static color_t bgcol;

static font_t fonts[FONTS] = {
    [FONT_SANS_S] = {.glyphs = (glyph_t *)bakedInterDisplayRegular12,
                     .nglyphs = sizeof(bakedInterDisplayRegular12),
                     .bitmap = atlasInterDisplayRegular12,
                     .nbitmap = sizeof(atlasInterDisplayRegular12)},
    [FONT_SANS_M] = {.glyphs = (glyph_t *)bakedInterDisplayRegular14,
                     .nglyphs = sizeof(bakedInterDisplayRegular14),
                     .bitmap = atlasInterDisplayRegular14,
                     .nbitmap = sizeof(atlasInterDisplayRegular14)},
    [FONT_SANS_BOLD_M] = {.glyphs = (glyph_t *)bakedInterDisplaySemiBold14,
                          .nglyphs = sizeof(bakedInterDisplaySemiBold14),
                          .bitmap = atlasInterDisplaySemiBold14,
                          .nbitmap = sizeof(atlasInterDisplaySemiBold14)},
    [FONT_ICONS_M] = {.glyphs = (glyph_t *)bakedIcons14,
                      .nglyphs = sizeof(bakedIcons14),
                      .bitmap = atlasIcons14,
                      .nbitmap = sizeof(atlasIcons14)},
    [FONT_ICONS_L] = {.glyphs = (glyph_t *)bakedIcons20,
                      .nglyphs = sizeof(bakedIcons20),
                      .bitmap = atlasIcons20,
                      .nbitmap = sizeof(atlasIcons20)},
    [FONT_ICONS_XL] = {.glyphs = (glyph_t *)bakedIcons32,
                      .nglyphs = sizeof(bakedIcons32),
                      .bitmap = atlasIcons32,
                      .nbitmap = sizeof(atlasIcons32)},
    [FONT_MONO_S] = {.glyphs = (glyph_t *)bakedJetBrainsMonoRegular12,
                     .nglyphs = sizeof(bakedJetBrainsMonoRegular12),
                     .bitmap = atlasJetBrainsMonoRegular12,
                     .nbitmap = sizeof(atlasJetBrainsMonoRegular12)},
    [FONT_MONO_M] = {.glyphs = (glyph_t *)bakedJetBrainsMonoRegular14,
                     .nglyphs = sizeof(bakedJetBrainsMonoRegular14),
                     .bitmap = atlasJetBrainsMonoRegular14,
                     .nbitmap = sizeof(atlasJetBrainsMonoRegular14)},
    [FONT_MONO_L] = {.glyphs = (glyph_t *)bakedJetBrainsMonoRegular20,
                     .nglyphs = sizeof(bakedJetBrainsMonoRegular20),
                     .bitmap = atlasJetBrainsMonoRegular20,
                     .nbitmap = sizeof(atlasJetBrainsMonoRegular20)}};

static sg_pipeline pipe;
static sg_sampler smp;
static sg_image atlases[FONTS];
static sg_view atlasviews[FONTS];

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
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = logger,
  });

  sg_shader shd = sg_make_shader(shader_shader_desc(sg_query_backend()));

  sg_pipeline_desc pipdesc = {0};
  pipdesc.shader = shd;
  pipdesc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
  pipdesc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
  pipdesc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4;
  pipdesc.colors[0].blend.enabled = true;
  pipdesc.colors[0].blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
  pipdesc.colors[0].blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  pipdesc.colors[0].write_mask = SG_COLORMASK_RGB;
  pipe = sg_make_pipeline(&pipdesc);

  for (int font = 0; font < FONTS; font++) {
    atlases[font] = sg_make_image(&(sg_image_desc){
        .pixel_format = SG_PIXELFORMAT_R8,
        .width = 512,
        .height = 512,
        .data = {.mip_levels = {{.ptr = fonts[font].bitmap,
                                 .size = fonts[font].nbitmap}}},
    });
    atlasviews[font] = sg_make_view(&(sg_view_desc){
        .texture.image = atlases[font],
    });
  }

  for (int i = 0; i < QUADS; i++) {
    buf[i] = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = sizeof(quads[i].buf),
    });
  }

  sg_sampler_desc smpdesc = {
      .min_filter = SG_FILTER_LINEAR,
      .mag_filter = SG_FILTER_LINEAR,
      .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
      .wrap_v = SG_WRAP_CLAMP_TO_EDGE,
  };
  smp = sg_make_sampler(&smpdesc);
}

void gfx_deinit(void) { sg_shutdown(); }

void gfx_drawstart(color_t color) {
  for (int i = 0; i < QUADS; i++) {
    quads[i].len = 0;
  }
  bgcol = color;
}

void gfx_drawend(void) {
#define draw(qs, b, v)                                                         \
  {                                                                            \
    size_t nverts = qs.len * VERTS_PER_QUAD;                                   \
    if (nverts) {                                                              \
      sg_range range = {0};                                                    \
      range.ptr = qs.buf;                                                      \
      range.size = nverts * VERT_SIZE * sizeof(float);                         \
      sg_update_buffer(b, &range);                                             \
                                                                               \
      sg_bindings bindings = {0};                                              \
      bindings.vertex_buffers[0] = b;                                          \
      bindings.views[VIEW_atlas] = v;                                          \
      bindings.samplers[SMP_smp] = smp;                                        \
                                                                               \
      sg_apply_pipeline(pipe);                                                 \
      sg_apply_bindings(&bindings);                                            \
      sg_draw(0, (int)nverts, 1);                                              \
    }                                                                          \
  }

  sg_pass pass = {0};
  pass.swapchain = sglue_swapchain();
  pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
  pass.action.colors[0].clear_value =
      (sg_color){bgcol.r, bgcol.g, bgcol.b, bgcol.a};
  sg_begin_pass(&pass);

  // Any view would work for rectangles (we are just taking a black dot)
  // We can just pick the first one
  draw(quads[RECT], buf[RECT], atlasviews[0]);
  for (int i = 0; i < FONTS; i++)
    draw(quads[i], buf[i], atlasviews[i]);

  sg_end_pass();
  sg_commit();
#undef gfx_drawquads
}

void gfx_rect(float x, float y, float w, float h, color_t col) {
  float scale = sapp_dpi_scale();
  float winw = sapp_widthf();
  float winh = sapp_heightf();

  // GPU coordinates are in NDC (normalized device coordinates) which is in the
  // -1, +1 range. The 2.0f scaling is used to map [0,1] -> [-1 +1]
  float l = (x * scale / winw) * 2.0f - 1.0f;
  float r = ((x + w) * scale / winw) * 2.0f - 1.0f;
  float t = 1.0f - (y * scale / winh) * 2.0f;
  float b = 1.0f - ((y + h) * scale / winh) * 2.0f;

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

  size_t off = quads[RECT].len * VERT_SIZE * VERTS_PER_QUAD;
  for (size_t i = 0; i < VERTS_PER_QUAD * VERT_SIZE; i++)
    quads[RECT].buf[off + i] = quad[i];
  quads[RECT].len++;
}

// Draws a glyph and return the cursor offset to draw the next one
static float gfx_glyph(float x, float y, char c, fonttype_t font, color_t col) {
  glyph_t g = fonts[font].glyphs[c - 32];

  float scale = sapp_dpi_scale();
  float winw = sapp_widthf();
  float winh = sapp_heightf();

  // x,y are logical pixels; g.xoff/yoff are physical pixels
  // Scale cursor to physical, then add the already-physical offset
  float px = x * scale + g.xoff;
  float py = y * scale + g.yoff;

  // GPU coordinates are in NDC (normalized device coordinates) which is in the
  // -1, +1 range. The 2.0f scaling is used to map [0,1] -> [-1 +1]
  float gw = (float)(g.x1 - g.x0) / winw * 2.0f;
  float gh = (float)(g.y1 - g.y0) / winh * 2.0f;
  float xyl = px / winw * 2.0f - 1.0f;
  float xyt = 1.0f - py / winh * 2.0f;
  float xyr = xyl + gw;
  float xyb = xyt - gh;
  float uvl = g.x0 / 512.0f;
  float uvt = g.y0 / 512.0f;
  float uvr = g.x1 / 512.0f;
  float uvb = g.y1 / 512.0f;

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

  size_t off = quads[font].len * VERT_SIZE * VERTS_PER_QUAD;
  for (size_t i = 0; i < VERTS_PER_QUAD * VERT_SIZE; i++)
    quads[font].buf[off + i] = quad[i];
  quads[font].len++;

  return g.xadvance / scale;
}

void gfx_text(float x, float y, string_t str, textdesc_t desc, color_t color) {
  float offset = 0;
  for (size_t i = 0; i < str.len; i++) {
    float hspacing = i == str.len - 1 ? 0 : desc.hspacing;
    offset += gfx_glyph(x + offset, y, str.buf[i], desc.font, color) + hspacing;
  }
}

void gfx_textbox(string_t str, textdesc_t desc, float *w, float *h) {
  float scale = sapp_dpi_scale();
  *w = 0;
  *h = 0;
  for (size_t i = 0; i < str.len; i++) {
    glyph_t g = fonts[desc.font].glyphs[str.buf[i] - 32];
    float hspacing = i == str.len - 1 ? 0 : desc.hspacing;
    *w += (g.xadvance / scale) + hspacing;
    float gh = (g.y1 - g.y0) / scale;
    *h = (gh > *h) ? gh : *h;
  }
}
