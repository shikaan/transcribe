#include "gfx.h"

#include <assert.h>
#include <math.h>
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <stddef.h>
#include <stdio.h>

#include "fonts/Icons14.h"
#include "fonts/Icons20.h"
#include "fonts/Icons32.h"
#include "fonts/InterDisplayRegular12.h"
#include "fonts/InterDisplayRegular14.h"
#include "fonts/InterDisplaySemiBold14.h"
#include "fonts/JetBrainsMonoRegular12.h"
#include "fonts/JetBrainsMonoRegular14.h"
#include "fonts/JetBrainsMonoRegular20.h"

#include "fonts/Icons14hidpi.h"
#include "fonts/Icons20hidpi.h"
#include "fonts/Icons32hidpi.h"
#include "fonts/InterDisplayRegular12hidpi.h"
#include "fonts/InterDisplayRegular14hidpi.h"
#include "fonts/InterDisplaySemiBold14hidpi.h"
#include "fonts/JetBrainsMonoRegular12hidpi.h"
#include "fonts/JetBrainsMonoRegular14hidpi.h"
#include "fonts/JetBrainsMonoRegular20hidpi.h"
#include "shader.h"
#include "utils.h"

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
static font_t fonts[FONTS] = {0};
static bool usehidpi = false;

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

static void fonts_setup(bool hidpi) {
  if (hidpi) {
    fonts[FONT_SANS_S]      = (font_t){.glyphs = (glyph_t *)bakedInterDisplayRegularhidpi12,  .nglyphs = sizeof(bakedInterDisplayRegularhidpi12),  .bitmap = atlasInterDisplayRegularhidpi12,  .nbitmap = sizeof(atlasInterDisplayRegularhidpi12)};
    fonts[FONT_SANS_M]      = (font_t){.glyphs = (glyph_t *)bakedInterDisplayRegularhidpi14,  .nglyphs = sizeof(bakedInterDisplayRegularhidpi14),  .bitmap = atlasInterDisplayRegularhidpi14,  .nbitmap = sizeof(atlasInterDisplayRegularhidpi14)};
    fonts[FONT_SANS_BOLD_M] = (font_t){.glyphs = (glyph_t *)bakedInterDisplaySemiBoldhidpi14, .nglyphs = sizeof(bakedInterDisplaySemiBoldhidpi14), .bitmap = atlasInterDisplaySemiBoldhidpi14, .nbitmap = sizeof(atlasInterDisplaySemiBoldhidpi14)};
    fonts[FONT_ICONS_M]     = (font_t){.glyphs = (glyph_t *)bakedIconshidpi14,                .nglyphs = sizeof(bakedIconshidpi14),                .bitmap = atlasIconshidpi14,                .nbitmap = sizeof(atlasIconshidpi14)};
    fonts[FONT_ICONS_L]     = (font_t){.glyphs = (glyph_t *)bakedIconshidpi20,                .nglyphs = sizeof(bakedIconshidpi20),                .bitmap = atlasIconshidpi20,                .nbitmap = sizeof(atlasIconshidpi20)};
    fonts[FONT_ICONS_XL]    = (font_t){.glyphs = (glyph_t *)bakedIconshidpi32,                .nglyphs = sizeof(bakedIconshidpi32),                .bitmap = atlasIconshidpi32,                .nbitmap = sizeof(atlasIconshidpi32)};
    fonts[FONT_MONO_S]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegularhidpi12, .nglyphs = sizeof(bakedJetBrainsMonoRegularhidpi12), .bitmap = atlasJetBrainsMonoRegularhidpi12, .nbitmap = sizeof(atlasJetBrainsMonoRegularhidpi12)};
    fonts[FONT_MONO_M]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegularhidpi14, .nglyphs = sizeof(bakedJetBrainsMonoRegularhidpi14), .bitmap = atlasJetBrainsMonoRegularhidpi14, .nbitmap = sizeof(atlasJetBrainsMonoRegularhidpi14)};
    fonts[FONT_MONO_L]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegularhidpi20, .nglyphs = sizeof(bakedJetBrainsMonoRegularhidpi20), .bitmap = atlasJetBrainsMonoRegularhidpi20, .nbitmap = sizeof(atlasJetBrainsMonoRegularhidpi20)};
  } else {
    fonts[FONT_SANS_S]      = (font_t){.glyphs = (glyph_t *)bakedInterDisplayRegular12,       .nglyphs = sizeof(bakedInterDisplayRegular12),       .bitmap = atlasInterDisplayRegular12,       .nbitmap = sizeof(atlasInterDisplayRegular12)};
    fonts[FONT_SANS_M]      = (font_t){.glyphs = (glyph_t *)bakedInterDisplayRegular14,       .nglyphs = sizeof(bakedInterDisplayRegular14),       .bitmap = atlasInterDisplayRegular14,       .nbitmap = sizeof(atlasInterDisplayRegular14)};
    fonts[FONT_SANS_BOLD_M] = (font_t){.glyphs = (glyph_t *)bakedInterDisplaySemiBold14,      .nglyphs = sizeof(bakedInterDisplaySemiBold14),      .bitmap = atlasInterDisplaySemiBold14,      .nbitmap = sizeof(atlasInterDisplaySemiBold14)};
    fonts[FONT_ICONS_M]     = (font_t){.glyphs = (glyph_t *)bakedIcons14,                     .nglyphs = sizeof(bakedIcons14),                     .bitmap = atlasIcons14,                     .nbitmap = sizeof(atlasIcons14)};
    fonts[FONT_ICONS_L]     = (font_t){.glyphs = (glyph_t *)bakedIcons20,                     .nglyphs = sizeof(bakedIcons20),                     .bitmap = atlasIcons20,                     .nbitmap = sizeof(atlasIcons20)};
    fonts[FONT_ICONS_XL]    = (font_t){.glyphs = (glyph_t *)bakedIcons32,                     .nglyphs = sizeof(bakedIcons32),                     .bitmap = atlasIcons32,                     .nbitmap = sizeof(atlasIcons32)};
    fonts[FONT_MONO_S]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegular12,       .nglyphs = sizeof(bakedJetBrainsMonoRegular12),       .bitmap = atlasJetBrainsMonoRegular12,       .nbitmap = sizeof(atlasJetBrainsMonoRegular12)};
    fonts[FONT_MONO_M]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegular14,       .nglyphs = sizeof(bakedJetBrainsMonoRegular14),       .bitmap = atlasJetBrainsMonoRegular14,       .nbitmap = sizeof(atlasJetBrainsMonoRegular14)};
    fonts[FONT_MONO_L]      = (font_t){.glyphs = (glyph_t *)bakedJetBrainsMonoRegular20,       .nglyphs = sizeof(bakedJetBrainsMonoRegular20),       .bitmap = atlasJetBrainsMonoRegular20,       .nbitmap = sizeof(atlasJetBrainsMonoRegular20)};
  }

  for (int font = 0; font < FONTS; font++) {
    sg_destroy_image(atlases[font]);
    sg_destroy_view(atlasviews[font]);

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

    float top = 0;
    float bot = 0;
    size_t ng = fonts[font].nglyphs / sizeof(glyph_t);
    for (size_t i = 0; i < ng; i++) {
      glyph_t g = fonts[font].glyphs[i];
      if (g.x1 == g.x0 && g.y1 == g.y0)
        continue;
      float gtop = g.yoff;
      float gbot = g.yoff + (float)(g.y1 - g.y0);
      if (gtop < top)
        top = gtop;
      if (gbot > bot)
        bot = gbot;
    }
    fonts[font].asc = -top;
    fonts[font].desc = bot;
  }

  usehidpi = hidpi;
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

  fonts_setup(sapp_dpi_scale() >= 1.5F);

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
  bool hidpi = sapp_dpi_scale() >= 1.5F;
  if (hidpi != usehidpi)
    fonts_setup(hidpi);

  for (int i = 0; i < QUADS; i++) {
    quads[i].len = 0;
  }
  bgcol = color;
}

void gfx_drawend(void) {
#define draw(qs, b, v)                                                         \
  {                                                                            \
    size_t nverts = (qs).len * VERTS_PER_QUAD;                                 \
    if (nverts) {                                                              \
      sg_range range = {0};                                                    \
      range.ptr = (qs).buf;                                                    \
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
  // sglue_swapchain is a frame-pacing function. It calls sapp_get_swapchain
  // and waits until the display has a free drawable to render into. It's a
  // vsync block which takes all the time untim 16ms (at 60fps). Remove it from
  // performance calculations.
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
#undef draw
}

void gfx_rect(float x, float y, float w, float h, color_t col) {
  float scale = sapp_dpi_scale();
  float winw = sapp_widthf();
  float winh = sapp_heightf();

  // GPU coordinates are in NDC (normalized device coordinates) which is in the
  // -1, +1 range. The 2.0f scaling is used to map [0,1] -> [-1 +1]
  float l = ((x * scale / winw) * 2.0F) - 1.0F;
  float r = (((x + w) * scale / winw) * 2.0F) - 1.0F;
  float t = 1.0F - ((y * scale / winh) * 2.0F);
  float b = 1.0F - (((y + h) * scale / winh) * 2.0F);

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
  for (size_t i = 0; i < len(quad); i++)
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
  // FIXME: rounding is needed for low-res screen to render fonts better
  //        however, glyphs that are too thin still don't render perfectly...
  float px = roundf((x * scale) + g.xoff);
  float py = roundf((y * scale) + g.yoff);

  // GPU coordinates are in NDC (normalized device coordinates) which is in the
  // -1, +1 range. The 2.0f scaling is used to map [0,1] -> [-1 +1]
  float gw = (float)(g.x1 - g.x0) / winw * 2.0F;
  float gh = (float)(g.y1 - g.y0) / winh * 2.0F;
  float xyl = (px / winw * 2.0F) - 1.0F;
  float xyt = 1.0F - (py / winh * 2.0F);
  float xyr = xyl + gw;
  float xyb = xyt - gh;
  float uvl = (float)g.x0 / 512.0F;
  float uvt = (float)g.y0 / 512.0F;
  float uvr = (float)g.x1 / 512.0F;
  float uvb = (float)g.y1 / 512.0F;

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
  for (size_t i = 0; i < len(quad); i++)
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

void gfx_measuretext(string_t str, textdesc_t desc, textmeasure_t *measure) {
  float scale = sapp_dpi_scale();
  float w = 0;
  float top = 0;
  float bot = 0;
  for (size_t i = 0; i < str.len; i++) {
    glyph_t g = fonts[desc.font].glyphs[str.buf[i] - 32];
    float hspacing = i == str.len - 1 ? 0 : desc.hspacing;
    w += (g.xadvance / scale) + hspacing;
    float gtop = g.yoff / scale;
    float gbot = (g.yoff + (float)(g.y1 - g.y0)) / scale;
    if (gtop < top)
      top = gtop;
    if (gbot > bot)
      bot = gbot;
  }
  measure->w = w;
  measure->h = bot - top;
  measure->asc = fonts[desc.font].asc / scale;
  measure->desc = fonts[desc.font].desc / scale;
}

color_t gfx_darken(color_t c, float amount) {
  assert(amount >= 0 && amount <= 1.0F &&
         "gfx_darken: percentage should be in [0,1]");
  return (color_t){
      c.r * (1 - amount),
      c.g * (1 - amount),
      c.b * (1 - amount),
      c.a,
  };
}

color_t gfx_lighten(color_t c, float amount) {
  assert(amount >= 0 && amount <= 1.0F &&
         "gfx_lighten: percentage should be in [0,1]");
  return (color_t){
      c.r + ((1 - c.r) * amount),
      c.g + ((1 - c.g) * amount),
      c.b + ((1 - c.b) * amount),
      c.a,
  };
}

color_t gfx_setalpha(color_t c, float amount) {
  assert(amount >= 0 && amount <= 1.0F &&
         "gfx_setalpha: percentage should be in [0,1]");
  return (color_t){
      c.r,
      c.g,
      c.b,
      amount,
  };
}

void gfx_line(float x0, float y0, float len, linedir_t dir, color_t col) {
  const float thickness = 1;
  gfx_rect(x0, y0, dir == LINEDIR_V ? thickness : len,
           dir == LINEDIR_V ? len : thickness, col);
}
