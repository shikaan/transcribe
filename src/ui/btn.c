#pragma once

#include "gfx/gfx.h"
#include "input/input.h"
#include "types.h"
#include "ui/theme.h"
#include "utils.h"
#include <stdbool.h>

typedef struct {
  string_t icon;
  fonttype_t iconfont;
  string_t label;
  fonttype_t labelfont;
  color_t background;
  color_t foreground;
  float pady;
  void (*onclick)(void);
} btndesc_t;

void ui_btn(float x, float y, float w, float h, btndesc_t desc) {
  const bool has_icon = desc.icon.len > 0;
  const bool has_label = desc.label.len > 0;

  float icow = 0, icoh = 0;
  textdesc_t icodesc = {desc.iconfont, 0};
  if (has_icon) gfx_textbox(desc.icon, icodesc, &icow, &icoh);

  float txtw = 0, txth = 0;
  textdesc_t txtdesc = {desc.labelfont, 0};
  if (has_label) gfx_textbox(desc.label, txtdesc, &txtw, &txth);

  color_t background = desc.background;
  if (overlaps(x, y, w, h, input.mousex, input.mousey)) {
    background = input.down ? gfx_lighten(background, 0.08f)
                            : gfx_lighten(background, 0.15f);
    if (input.clicked && desc.onclick) desc.onclick();
  }
  gfx_rect(x, y, w, h, background);

  float gap = (has_icon && has_label) ? theme->spacing : 0;
  float totalw = icow + gap + txtw;
  float currx = x + w / 2 - totalw / 2;
  float midy = y + h / 2;

  if (has_icon)
    gfx_text(currx, midy + icoh / 2 + desc.pady, desc.icon, icodesc,
             desc.foreground);
  if (has_label)
    gfx_text(currx + icow + gap, midy + txth / 2 + desc.pady, desc.label,
             txtdesc, desc.foreground);
}
