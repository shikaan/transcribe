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
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float height = theme->spacing * 6;
  const float padx = theme->spacing * 3;
  const float iconspace = theme->spacing;
  const float spacebetween = theme->spacing * 2;
  const float sepheight = theme->spacing * 2;
  float currentx = 0;
  float txtw = 0, txth = 0;

  gfx_rect(currentx, 0, winw, height, theme->colors[COLOR_BARBACKGROUND]);

  string_t puzzle = fromconst("c");
  gfx_textbox(puzzle, FONT_ICONS_M, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, puzzle, FONT_ICONS_M,
           theme->colors[COLOR_PRIMARY]);

  string_t logo = fromconst("Transcribling");
  currentx += iconspace + txtw;
  gfx_textbox(logo, FONT_SANS_BOLD_M, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, logo, FONT_SANS_BOLD_M,
           theme->colors[COLOR_FOREGROUND]);

  currentx += txtw + padx;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t openicon = fromconst("a");
  gfx_textbox(openicon, FONT_ICONS_M, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, openicon, FONT_ICONS_M,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t open = fromconst("Open");
  currentx += iconspace + txtw;
  gfx_textbox(open, FONT_SANS_M, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, open, FONT_SANS_M,
           theme->colors[COLOR_TEXTLIGHT]);

  currentx += txtw + spacebetween;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t fileicon = fromconst("b");
  gfx_textbox(fileicon, FONT_ICONS_M, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, fileicon, FONT_ICONS_M,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t file = fromconst("Miles Davis - So What.mp3");
  currentx += iconspace + txtw;
  gfx_textbox(logo, FONT_SANS_M, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, file, FONT_SANS_M,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t settingsicon = fromconst("l");
  gfx_textbox(settingsicon, FONT_ICONS_M, &txtw, &txth);
  gfx_text(winw - txtw - spacebetween, height / 2 + txth / 2, settingsicon,
           FONT_ICONS_M, theme->colors[COLOR_TEXTLIGHT]);

  ui_line(0, height, winw, LINEDIR_H, theme->colors[COLOR_TEXTDIM]);
}
