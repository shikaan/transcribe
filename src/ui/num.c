#include "gfx/gfx.h"
#include "types.h"
#include "ui/btn.c"
#include "ui/theme.h"
#include <stdio.h>

typedef struct {
  void (*onincrease)(void);
  void (*ondecrease)(void);
  string_t label;
  string_t value;
  bool changed;
} numdesc_t;

void ui_num(float x, float y, float w, float h, numdesc_t desc) {
  float txtw;
  float txth;
  textdesc_t ldesc = {FONT_SANS_S, 2.5F};
  gfx_textbox(desc.label, ldesc, &txtw, &txth);
  gfx_text(x + (w / 2) - (txtw / 2), y + txth, desc.label, ldesc,
           theme->colors[COLOR_TEXTLIGHT]);

  textdesc_t vdesc = {FONT_MONO_M, 0};
  color_t color = desc.changed ? theme->colors[COLOR_PRIMARY]
                                 : theme->colors[COLOR_TEXTLIGHT];
  gfx_textbox(desc.value, vdesc, &txtw, &txth);
  gfx_text(x + (w / 2) - (txtw / 2), y + h - (txth / 2), desc.value, vdesc,
           color);

  const float icosz = theme->spacing * 3;

  ui_btn(x, y + h - icosz, icosz, icosz,
         (btndesc_t){.iconfont = FONT_ICONS_M,
                     .icon = string(ICON_MINUS),
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .onclick = desc.ondecrease,
                     .pady = 4.0F});

  ui_btn(x + w - icosz, y + h - icosz, icosz, icosz,
         (btndesc_t){.iconfont = FONT_ICONS_M,
                     .icon = string(ICON_PLUS),
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .onclick = desc.onincrease});
}
