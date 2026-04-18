#pragma once

#include "gfx/gfx.h"
#include "input/input.h"
#include "types.h"
#include "ui/theme.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  fonttype_t iconfont;
  fonttype_t labelfont;
  string_t icon;
  string_t label;
  color_t background;
  color_t foreground;
  float pady;
  bool disabled;
  void (*onclick)(void);
} btndesc_t;

void ui_btn(float x, float y, float w, float h, btndesc_t desc) {
  const bool has_icon = desc.icon.len > 0;
  const bool has_label = desc.label.len > 0;

  float icow = 0;
  float icoh = 0;
  textdesc_t icodesc = {desc.iconfont, 0};
  if (has_icon)
    gfx_textbox(desc.icon, icodesc, &icow, &icoh);

  float txtw = 0;
  float txth = 0;
  textdesc_t txtdesc = {desc.labelfont, 0};
  if (has_label)
    gfx_textbox(desc.label, txtdesc, &txtw, &txth);

  color_t background = desc.background;
  if (!desc.disabled && overlaps(x, y, w, h, input.mousex, input.mousey)) {
    input_setcursor(CURSOR_POINTER);
    background = input.down ? gfx_lighten(background, 0.08F)
                            : gfx_lighten(background, 0.15F);
    if (input_lclicked() && desc.onclick)
      desc.onclick();
  }
  gfx_rect(x, y, w, h, background);

  float gap = (has_icon && has_label) ? theme->spacing : 0;
  float totalw = icow + gap + txtw;
  float currx = x + (w / 2) - (totalw / 2);
  float midy = y + (h / 2);

  if (has_icon) {
    color_t foreground =
        desc.disabled ? gfx_setalpha(desc.foreground, 0.4F) : desc.foreground;
    gfx_text(currx, midy + (icoh / 2) + desc.pady, desc.icon, icodesc,
             foreground);
  }
  if (has_label) {
    color_t foreground =
        desc.disabled ? gfx_setalpha(desc.foreground, 0.15F) : desc.foreground;
    gfx_text(currx + icow + gap, midy + (txth / 2) + desc.pady, desc.label,
             txtdesc, foreground);
  }
}
