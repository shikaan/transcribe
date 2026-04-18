#include "gfx/gfx.h"
#include "types.h"
#include "ui/button.c"
#include "ui/theme.h"
#include <stdio.h>

typedef struct {
  void (*onincrease)(void);
  void (*ondecrease)(void);
  string_t label;
  string_t value;
  string_t incicon;
  string_t decicon;
  bool changed;
  bool incdisabled;
  bool decdisabled;
} stepperdesc_t;

void ui_stepper(float x, float y, float w, float h, stepperdesc_t desc) {
  const float icosz = theme->spacing * 3;
  textmeasure_t m = {0};

  if (desc.label.len > 0) {
    textdesc_t ldesc = {FONT_SANS_S, 2.5F};
    gfx_measuretext(desc.label, ldesc, &m);
    gfx_text(x + (w / 2) - (m.w / 2), y + m.fontasc, desc.label, ldesc,
             theme->colors[COLOR_TEXTLIGHT]);
  }

  textdesc_t vdesc = {FONT_MONO_M, 0};
  color_t color = desc.changed ? theme->colors[COLOR_PRIMARY]
                               : theme->colors[COLOR_TEXTLIGHT];
  gfx_measuretext(desc.value, vdesc, &m);
  float valmid = y + h - (icosz / 2);
  gfx_text(x + (w / 2) - (m.w / 2),
           valmid + ((m.fontasc - m.fontdesc) / 2), desc.value, vdesc, color);

  ui_button(x, y + h - icosz, icosz, icosz,
         (buttondesc_t){.iconfont = FONT_ICONS_M,
                     .icon = desc.decicon,
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .onclick = desc.ondecrease,
                     .disabled = desc.decdisabled});

  ui_button(x + w - icosz, y + h - icosz, icosz, icosz,
         (buttondesc_t){.iconfont = FONT_ICONS_M,
                     .icon = desc.incicon,
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .disabled = desc.incdisabled,
                     .onclick = desc.onincrease});
}
