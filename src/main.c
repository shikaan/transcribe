#include <stdio.h>

#include "../lib/sokol_app.h"
#include "gfx.h"

static void init(void) { gfx_init(); }
static void frame(void) {
  color_t bg = {0.5, 0.5, 0.5, 1};
  gfx_drawstart(bg);

  color_t green = {0, 1, 0, 1};
  gfx_rect(5, 5, 50, 50, green);
  color_t black = {0, 0, 0, 1};
  gfx_glyph(160, 160, 'a', black);
  gfx_drawend();
}
static void cleanup(void) { gfx_deinit(); }

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .window_title = "Transcribling",
      .width = 800,
      .height = 600,
  };
}
