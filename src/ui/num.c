#include "gfx/gfx.h"
#include "types.h"
#include "ui/btn.c"
#include "ui/theme.h"

void ui_num(float x, float y, float w, float h, string_t label,
            string_t value) {
  float txtw;
  float txth;
  textdesc_t ldesc = {FONT_SANS_S, 2.5F};
  gfx_textbox(label, ldesc, &txtw, &txth);
  gfx_text(x + (w / 2) - (txtw / 2), y + txth, label, ldesc,
           theme->colors[COLOR_TEXTLIGHT]);

  textdesc_t vdesc = {FONT_MONO_M, 0};
  gfx_textbox(value, vdesc, &txtw, &txth);
  gfx_text(x + (w / 2) - (txtw / 2), y + h - (txth / 2), value, vdesc,
           theme->colors[COLOR_TEXTLIGHT]);

  const float icosz = theme->spacing * 3;

  ui_btn(x, y + h - icosz, icosz, icosz,
         (btndesc_t){.iconfont = FONT_ICONS_M,
                     .icon = string(ICON_MINUS),
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .pady = 4.0F});

  ui_btn(x + w - icosz, y + h - icosz, icosz, icosz,
         (btndesc_t){.iconfont = FONT_ICONS_M,
                     .icon = string(ICON_PLUS),
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT]});
}
