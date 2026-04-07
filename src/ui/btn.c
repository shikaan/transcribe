#pragma once

#include "gfx/gfx.h"
#include "input/input.h"
#include "types.h"
#include "utils.h"
#include <stdbool.h>

typedef struct {
  string_t label;
  fonttype_t font;
  // FIXME: this is needed just because I cannot align - in inputs
  //        it's likely going to break somewhere else too if we don't understand
  //        how to properly center text vertically
  float pady;
  color_t background;
  color_t foreground;
  void (*onclick)(void);
} btndesc_t;

void ui_btn(float x, float y, float w, float h, btndesc_t desc) {
  float txtw, txth;
  textdesc_t btndesc = {desc.font, 0};
  gfx_textbox(desc.label, btndesc, &txtw, &txth);

  color_t background = desc.background;
  if (overlaps(x, y, w, h, input.mousex, input.mousey)) {
    background = input.down ? gfx_lighten(background, 0.08f)
                            : gfx_lighten(background, 0.15f);

    if (input.clicked && desc.onclick)
      desc.onclick();
  }

  gfx_rect(x, y, w, h, background);
  gfx_text(x + w / 2 - txtw / 2, y + h / 2 + txth / 2 + desc.pady, desc.label,
           btndesc, desc.foreground);
}
