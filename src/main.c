#include <assert.h>
#include <miniaudio.h>
#include <stdio.h>
#include <sokol_app.h>

#include "gfx/gfx.h"
#include "ui/theme.h"
#include "ui/ui.h"

static void init(void) {
  gfx_init();
  ui_init();
}

static void frame(void) {
  gfx_drawstart(theme->colors[COLOR_BG]);
  ui_titlebar();
  ui_timeline();
  ui_spectrum();
  ui_controls();
  ui_playhead();
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
      .width = 1024,
      .height = 768,
      .high_dpi = true,
  };
}
