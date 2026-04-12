#include <assert.h>
#include <sokol_app.h>
#include <stdio.h>

#include "audio/audio.h"
#include "gfx/gfx.h"
#include "input/input.h"
#include "prof/prof.h"
#include "ui/theme.h"
#include "ui/ui.h"

static void init(void) {
  #ifdef DEBUG
    audio_open("test1.mp3");
    audio_settempo(0.5);
  #endif
  gfx_init();
}

static void frame(void) {
  gfx_drawstart(theme->colors[COLOR_BACKGROUND]);
  ui_app();
  gfx_drawend();
  input_done();
}

static void cleanup(void) {
  gfx_deinit();
  prof_print();
}

sapp_desc sokol_main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  return (sapp_desc){.init_cb = init,
                     .frame_cb = frame,
                     .cleanup_cb = cleanup,
                     .event_cb = input_event,
                     .window_title = "Transcribling",
                     .width = 1024,
                     .height = 768,
                     .high_dpi = true};
}
