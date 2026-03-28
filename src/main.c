#include <stdio.h>
#include <string.h>

#include "../lib/sokol_app.h"
#include "gfx.h"
#include "theme.h"
#include "ui.h"

static void init(void) { gfx_init(); }
static void frame(void) {
  gfx_drawstart(theme->colors[COLOR_BACKGROUND]);
  ui_titlebar();
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
