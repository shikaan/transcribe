#include <stdio.h>
#include <string.h>

#include "../lib/sokol_app.h"
#include "gfx.h"
#include "slice.h"

static void init(void) { gfx_init(); }
static void frame(void) {
  color_t bg = {1, 1, 1, 1};
  gfx_drawstart(bg);

  color_t green = {0, 1, 0, 1};
  gfx_rect(5, 5, 50, 50, green);
  color_t black = {0, 0, 0, 1};

  char buf[] = "Hallo ich bin Manuel. Wer bist du?";
  string_t str;
  str.buf = buf;
  str.len = strlen(buf);

  float w, h;
  gfx_textbox(str, &w, &h);
  gfx_rect(160, 160 - h, w, h, green);
  gfx_text(160, 160, str, black);
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
      .high_dpi = true,
  };
}
