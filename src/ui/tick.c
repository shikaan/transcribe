#pragma once

#include "gfx/gfx.h"
#include "input/input.h"
#include "types.h"
#include "utils.h"
#include <stdbool.h>

typedef struct {
  string_t icon;
  fonttype_t iconfont;
  bool value;
  color_t background[2];
  color_t foreground[2];
  void (*onchange)(bool value);
} tickdesc_t;

void ui_tick(float x, float y, float w, float h, tickdesc_t desc) {
  float icow = 0, icoh = 0;
  textdesc_t icodesc = {desc.iconfont, 0};
  gfx_textbox(desc.icon, icodesc, &icow, &icoh);

  color_t background = desc.background[desc.value];
  if (overlaps(x, y, w, h, input.mousex, input.mousey)) {
    background = input.down ? gfx_lighten(background, 0.08f)
                            : gfx_lighten(background, 0.15f);
    if (input.clicked && desc.onchange) desc.onchange(!desc.value);
  }
  gfx_rect(x, y, w, h, background);

  float currx = x + w / 2 - icow / 2;
  float midy = y + h / 2;

  gfx_text(currx, midy + icoh / 2, desc.icon, icodesc,
           desc.foreground[desc.value]);
}
