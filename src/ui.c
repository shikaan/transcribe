#include "../lib/sokol_app.h"
#include "gfx.h"
#include "string.h"
#include "theme.h"

typedef enum {
  LINEDIR_H,
  LINEDIR_V,
} linedir_t;

static void ui_line(float x0, float y0, float len, linedir_t dir, color_t col) {
  const float thickness = 1;
  gfx_rect(x0, y0, dir == LINEDIR_V ? thickness : len,
           dir == LINEDIR_V ? len : thickness, col);
}

void ui_titlebar(void) {
  const float winw = sapp_widthf();
  const float scale = sapp_dpi_scale();
  const float height = theme->spacing * 5;
  const float padx = theme->spacing * 4;
  const float spacebetween = theme->spacing * 2;
  const float sepheight = theme->spacing * 2;
  float currentx = 0;
  float txtw = 0, txth = 0;

  gfx_rect(currentx, 0, winw / scale, height,
           theme->colors[COLOR_BARBACKGROUND]);

  string_t logo = fromconst("Transcribling");
  gfx_textbox(logo, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, logo,
           theme->colors[COLOR_FOREGROUND]);

  currentx += txtw + padx;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t open = fromconst("Open");
  gfx_textbox(open, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, open,
           theme->colors[COLOR_TEXTLIGHT]);

  currentx += txtw + spacebetween;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t file = fromconst("Miles Davis - So What.mp3");
  gfx_textbox(logo, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, file,
           theme->colors[COLOR_TEXTLIGHT]);

  ui_line(0, height, winw, LINEDIR_H, theme->colors[COLOR_TEXTDIM]);
}
