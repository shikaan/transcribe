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
  BARPADX = 3,

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
  const float padx = theme->spacing * BARPADX;
  const float iconspace = theme->spacing;
  const float spacebetween = theme->spacing * 2;
  const float sepheight = theme->spacing * 2;
  float txtw = 0, txth = 0;

  gfx_rect(0, 0, winw, height, theme->colors[COLOR_BARBG]);

  float currentx = 0;
  string_t puzzle = fromconst("c");
  textdesc_t desc = (textdesc_t){FONT_ICONS_M, 0};
  gfx_textbox(puzzle, desc, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, puzzle, desc,
           theme->colors[COLOR_PRIMARY]);

  string_t logo = fromconst("Transcribling");
  currentx += iconspace + txtw;
  desc.font = FONT_SANS_BOLD_M;
  gfx_textbox(logo, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, logo, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  currentx += txtw + padx;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t openicon = fromconst("a");
  desc.font = FONT_ICONS_M;
  gfx_textbox(openicon, desc, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, openicon, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t open = fromconst("Open");
  currentx += iconspace + txtw;
  desc.font = FONT_SANS_M;
  gfx_textbox(open, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, open, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  currentx += txtw + spacebetween;
  ui_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
          theme->colors[COLOR_TEXTDIM]);

  string_t fileicon = fromconst("b");
  desc.font = FONT_ICONS_M;
  gfx_textbox(fileicon, desc, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, fileicon, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t file = fromconst("Miles Davis - So What.mp3");
  currentx += iconspace + txtw;
  desc.font = FONT_SANS_M;
  gfx_textbox(logo, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, file, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t settingsicon = fromconst("m");
  desc.font = FONT_ICONS_M;
  gfx_textbox(settingsicon, desc, &txtw, &txth);
  gfx_text(winw - txtw - padx, height / 2 + txth / 2, settingsicon, desc,
           theme->colors[COLOR_TEXTLIGHT]);

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
  textdesc_t desc = {FONT_MONO_S, 0};
  for (float x = 0; x < winw; x += stride * 2) {
    gfx_text(x + linew, y - timepad, time, desc,
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

static void ui_number(float x, float y, float w, float h, string_t label,
                      string_t value) {
  float txtw, txth, icow, icoh;
  textdesc_t ldesc = {FONT_SANS_S, 2.5f};
  gfx_textbox(label, ldesc, &txtw, &txth);
  gfx_text(x + w / 2 - txtw / 2, y + txth, label, ldesc,
           theme->colors[COLOR_TEXTLIGHT]);

  textdesc_t vdesc = {FONT_MONO_M, 0};
  gfx_textbox(value, vdesc, &txtw, &txth);
  gfx_text(x + w / 2 - txtw / 2, y + h - txth / 2, value, vdesc,
           theme->colors[COLOR_TEXTLIGHT]);

  textdesc_t idesc = {FONT_ICONS_M, 0};
  string_t plus = fromconst("d");
  gfx_textbox(plus, idesc, &icow, &icoh);
  gfx_text(x, y + h - icoh / 4, plus, idesc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  string_t minus = fromconst("e");
  gfx_text(x + w - icow, y + h - icoh / 4, minus, idesc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);
}

void ui_controls(void) {
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float winh = sapp_heightf() / scale;
  const float height = theme->spacing * 12;
  const float padx = theme->spacing * BARPADX;
  const float y = winh - height;

  float txtw, txth, currentx = 0.0f;

  gfx_rect(0, y, winw, height, theme->colors[COLOR_BARBG]);

  string_t timecodenow = fromconst("01:23.456");
  currentx += padx;
  textdesc_t desc = {FONT_MONO_L, 0};
  gfx_textbox(timecodenow, desc, &txtw, &txth);
  gfx_text(currentx, y + height / 2 + txth / 2, timecodenow, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  string_t timecode = fromconst("01:23.456");
  currentx += txtw + theme->spacing * 2;
  desc.font = FONT_MONO_M;
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  const float numberh = theme->spacing * 6;
  const float numberw = theme->spacing * 12;
  const float spacebetween = theme->spacing * 4;

  currentx = winw - padx - numberw;
  ui_number(currentx, winh - (height + numberh) / 2, numberw, numberh,
            fromconst("PITCH"), fromconst("0 st"));

  currentx -= numberw + spacebetween;
  ui_number(currentx, winh - (height + numberh) / 2, numberw, numberh,
            fromconst("TEMPO"), fromconst("100%"));

  string_t play = fromconst("i");
  textdesc_t btndesc = {FONT_ICONS_XL, 0};
  gfx_textbox(play, btndesc, &txtw, &txth);
  const float playw = txtw;
  const float playx = winw / 2 - playw / 2;
  gfx_text(playx, y + height / 2 + txth / 2, play, btndesc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  string_t loop = fromconst("h");
  btndesc.font = FONT_ICONS_L;
  gfx_textbox(loop, btndesc, &txtw, &txth);
  gfx_text(winw / 2 + playw / 2 + spacebetween,
           y + height / 2 + txth / 2, loop, btndesc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  string_t stop = fromconst("k");
  btndesc.font = FONT_ICONS_L;
  gfx_text(winw / 2 - playw / 2 - txtw - spacebetween,
           y + height / 2 + txth / 2, stop, btndesc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);
}
