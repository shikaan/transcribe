#include <stdio.h>

#include "../lib/sokol_app.h"

static void init(void) {}
static void frame(void) {}
static void cleanup(void) {}

sapp_desc sokol_main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    return (sapp_desc){
        .init_cb    = init,
        .frame_cb   = frame,
        .cleanup_cb = cleanup,
        .window_title = "Transcribling",
        .width  = 800,
        .height = 600,
    };
}
