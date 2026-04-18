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
  bool disabled;
  void (*onclick)(void);
} buttondesc_t;

void ui_button(float x, float y, float w, float h, buttondesc_t desc) {
  const bool has_icon = desc.icon.len > 0;
  const bool has_label = desc.label.len > 0;

  textmeasure_t ico = {0};
  textdesc_t icodesc = {desc.iconfont, 0};
  if (has_icon)
    gfx_measuretext(desc.icon, icodesc, &ico);

  textmeasure_t txt = {0};
  textdesc_t txtdesc = {desc.labelfont, 0};
  if (has_label)
    gfx_measuretext(desc.label, txtdesc, &txt);

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
  float totalw = ico.w + gap + txt.w;
  float currx = x + (w / 2) - (totalw / 2);
  float midy = y + (h / 2);

  if (has_icon) {
    color_t foreground =
        desc.disabled ? gfx_setalpha(desc.foreground, 0.3F) : desc.foreground;
    gfx_text(currx, midy + ((ico.asc - ico.desc) / 2), desc.icon,
             icodesc, foreground);
  }
  if (has_label) {
    color_t foreground =
        desc.disabled ? gfx_setalpha(desc.foreground, 0.3F) : desc.foreground;
    gfx_text(currx + ico.w + gap, midy + ((txt.asc - txt.desc) / 2),
             desc.label, txtdesc, foreground);
  }
}
