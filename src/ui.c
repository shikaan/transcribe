#include "ui.h"
#include "../lib/sokol_app.h"
#include "gfx.h"
#include "string.h"
#include "theme.h"
#include <stdlib.h>
#include <time.h>

enum {
  TITLEBARH = 6,
  TIMELINEH = 4,
  SPECTRUM_H = 10,

  NSAMPLES = 1024
};

typedef enum {
  LINEDIR_H,
  LINEDIR_V,
} linedir_t;

static void ui_line(float x0, float y0, float len, linedir_t dir, color_t col) {
  const float thickness = 1;
  gfx_rect(x0, y0, dir == LINEDIR_V ? thickness : len,
           dir == LINEDIR_V ? len : thickness, col);
}

float samples[NSAMPLES] = {0};
void ui_init(void) {
  srand((unsigned)time(NULL));
  for (int i = 0; i < NSAMPLES; i++) {
    samples[i] = (float)rand() / RAND_MAX;
  }
}

void ui_titlebar(void) {
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float height = theme->spacing * TITLEBARH;
  const float padx = theme->spacing * 3;
  const float iconspace = theme->spacing;
  const float spacebetween = theme->spacing * 2;
  const float sepheight = theme->spacing * 2;
  float txtw = 0, txth = 0;

  gfx_rect(0, 0, winw, height, theme->colors[COLOR_BARBG]);

  float currentx = 0;
  string_t puzzle = fromconst("c");
  gfx_textbox(puzzle, FONT_ICONS_M, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, puzzle, FONT_ICONS_M,
           theme->colors[COLOR_PRIMARY]);

  string_t logo = fromconst("Transcribling");
  currentx += iconspace + txtw;
  gfx_textbox(logo, FONT_SANS_BOLD_M, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, logo, FONT_SANS_BOLD_M,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

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

  ui_line(0, height - 1, winw, LINEDIR_H, theme->colors[COLOR_TEXTDIM]);
}

void ui_timeline(void) {
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float timeliney = theme->spacing * TITLEBARH;
  const float height = theme->spacing * TIMELINEH;
  const float lineh = theme->spacing;
  const float linew = 1;
  const float llineh = theme->spacing * 1.5f;
  const float timepad = theme->spacing / 4;
  const float stride = theme->spacing * 8;

  gfx_rect(0, timeliney, winw, height, theme->colors[COLOR_BARBG]);
  ui_line(0, timeliney + height - 1, winw, LINEDIR_H,
          theme->colors[COLOR_TEXTDIM]);

  string_t time = fromconst("00:00");
  float y = timeliney + height - 1 - llineh;
  for (float x = 0; x < winw; x += stride * 2) {
    gfx_text(x + linew, y - timepad, time, FONT_MONO_S,
             theme->colors[COLOR_TEXTLIGHT]);
    ui_line(x, y, llineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
  }

  y = timeliney + height - 1 - lineh;
  for (float x = stride; x < winw; x += stride * 2) {
    ui_line(x, y, lineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
  }
}

void ui_spectrum(void) {
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float topbary = theme->spacing * (TITLEBARH + TIMELINEH);
  const float height = theme->spacing * 32;

  const float barw = theme->spacing / 2;
  const float barmaxh = height - theme->spacing;
  const float bargap = theme->spacing / 4;

  gfx_rect(0, topbary, winw, height, theme->colors[COLOR_SPECTRUMBG]);

  int i = 0;
  float x = 0;
  for (; x < winw && i < NSAMPLES; x += barw + bargap, i++) {
    float barh = barmaxh * samples[i];
    gfx_rect(x, topbary + (height - barh) / 2, barw, barh,
             theme->colors[COLOR_TEXTLIGHT]);
  }
}
