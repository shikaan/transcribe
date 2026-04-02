#include "ui.h"
#include "audio/audio.h"
#include "gfx/gfx.h"
#include "input/input.h"
#include "prof/prof.h"
#include "theme.h"
#include "types.h"
#include <sokol_app.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const float TITLEBARH = 6;
const float TIMELINEH = 4;
const float SPECTRUM_H = 32;
const float BARPADX = 3;

typedef enum {
  LINEDIR_H,
  LINEDIR_V,
} linedir_t;

static void ui_line(float x0, float y0, float len, linedir_t dir, color_t col) {
  const float thickness = 1;
  gfx_rect(x0, y0, dir == LINEDIR_V ? thickness : len,
           dir == LINEDIR_V ? len : thickness, col);
}

static bool overlaps(float rx, float ry, float rw, float rh, float px,
                     float py) {
  return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

typedef struct {
  string_t label;
  fonttype_t font;
  void (*onclick)(void); // FIXME: this should take input_t
  color_t background;
  color_t foreground;
  // FIXME: this is needed just because I cannot align - in inputs
  //        it's likely going to break somewhere else too if we don't understand
  //        how to properly center text vertically
  float pady;
} btndesc_t;

// FIXME: if this becomes a performance issue, we could precompute the colors
//        instead of doing it on every frame
static void ui_btn(float x, float y, float w, float h, btndesc_t desc) {
  float txtw, txth;
  textdesc_t btndesc = {desc.font, 0};
  gfx_textbox(desc.label, btndesc, &txtw, &txth);

  color_t background = desc.background;
  if (overlaps(x, y, w, h, input.mousex, input.mousey)) {
    background = input.down ? gfx_lighten(background, 0.08f)
                            : gfx_lighten(background, 0.15f);

    if (input.clicked && desc.onclick)
      desc.onclick();
  }

  gfx_rect(x, y, w, h, background);
  gfx_text(x + w / 2 - txtw / 2, y + h / 2 + txth / 2 + desc.pady, desc.label,
           btndesc, desc.foreground);
}

static void ui_number(float x, float y, float w, float h, string_t label,
                      string_t value) {
  float txtw, txth;
  textdesc_t ldesc = {FONT_SANS_S, 2.5f};
  gfx_textbox(label, ldesc, &txtw, &txth);
  gfx_text(x + w / 2 - txtw / 2, y + txth, label, ldesc,
           theme->colors[COLOR_TEXTLIGHT]);

  textdesc_t vdesc = {FONT_MONO_M, 0};
  gfx_textbox(value, vdesc, &txtw, &txth);
  gfx_text(x + w / 2 - txtw / 2, y + h - txth / 2, value, vdesc,
           theme->colors[COLOR_TEXTLIGHT]);

  const float icosz = theme->spacing * 3;

  ui_btn(x, y + h - icosz, icosz, icosz,
         (btndesc_t){.font = FONT_ICONS_M,
                     .label = fromconst(ICON_MINUS),
                     .background = theme->colors[COLOR_BARBG],
                     .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
                     .pady = 4.0f});

  ui_btn(x + w - icosz, y + h - icosz, icosz, icosz,
         (btndesc_t){
             .font = FONT_ICONS_M,
             .label = fromconst(ICON_PLUS),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
         });
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

  string_t openicon = fromconst(ICON_OPEN);
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

  string_t fileicon = fromconst(ICON_FILE);
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

  string_t settingsicon = fromconst(ICON_EQ);
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
  const float spectrumy = theme->spacing * (TITLEBARH + TIMELINEH);
  const float spectrumx = 0;
  const float height = theme->spacing * SPECTRUM_H;

  const float barw = theme->spacing / 2;
  const float barminh = theme->spacing / 2;
  const float barmaxh = height - theme->spacing - barminh;
  const float bargap = theme->spacing / 4;

  gfx_rect(spectrumx, spectrumy, winw, height, theme->colors[COLOR_SPECTRUMBG]);

  unsigned int i = 0;
  float x = spectrumx;
  samples_t *samples = audio_samples();
  for (; x < winw && i < samples->len; x += barw + bargap, i++) {
    float barh = barmaxh * samples->buf[i] + barminh;
    gfx_rect(x, spectrumy + (height - barh) / 2, barw, barh,
             theme->colors[COLOR_TEXTLIGHT]);
  }

  const float heady = theme->spacing * (TITLEBARH + TIMELINEH);
  const float cursorw = theme->spacing;
  const float thickness = theme->spacing / 4;

  // FIXME: we could get the time once per frame instead of here and in controls
  float total, current;
  audio_times(&total, &current);
  const float tracklen = (float)samples->len * (barw + bargap);

  float headx = tracklen * current / total;
  if (input.pressed &&
      overlaps(spectrumx, spectrumy, winw, height, input.mousex, input.mousey)) {
    audio_seek(input.mousex * total / tracklen);
    headx = input.mousex;
  }

  if (headx < winw) {
    gfx_rect(headx - cursorw / 2 + thickness / 2, heady, cursorw, cursorw,
             theme->colors[COLOR_PRIMARY]);
    gfx_rect(headx, heady, thickness, height, theme->colors[COLOR_PRIMARY]);
  }
}

void ui_controls(void) {
  const float scale = sapp_dpi_scale();
  const float winw = sapp_widthf() / scale;
  const float winh = sapp_heightf() / scale;
  const float height = theme->spacing * 12;
  const float padx = theme->spacing * BARPADX;
  const float y = winh - height;

  float txtw, txth, currentx = 0.0f;
  float total, current;
  audio_times(&total, &current);

  gfx_rect(0, y, winw, height, theme->colors[COLOR_BARBG]);

  static char timecodebuf[10] = "00:00.000";
  static string_t timecode = {timecodebuf, 9};
  audio_sectotime(current, timecodebuf);

  currentx += padx;
  textdesc_t desc = {FONT_MONO_L, 0};
  gfx_textbox(timecode, desc, &txtw, &txth);
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  audio_sectotime(total, timecodebuf);

  currentx += txtw + theme->spacing * 2;
  desc.font = FONT_MONO_M;
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  const float numberh = theme->spacing * 5;
  const float numberw = theme->spacing * 14;
  float spacebetween = theme->spacing * 4;

  currentx = winw - padx - numberw;
  ui_number(currentx, winh - (height + numberh) / 2, numberw, numberh,
            fromconst("PITCH"), fromconst("0 st"));

  currentx -= numberw + spacebetween;
  ui_number(currentx, winh - (height + numberh) / 2, numberw, numberh,
            fromconst("TEMPO"), fromconst("100%"));

  const float primarybtnw = 64;
  const float primarybtnh = 64;
  const float secondarybtnw = 48;
  const float secondarybtnh = 48;
  spacebetween = theme->spacing * 2;

  if (!audio_isplaying()) {
    ui_btn(winw / 2 - primarybtnw / 2, y + height / 2 - primarybtnh / 2,
           primarybtnw, primarybtnh,
           (btndesc_t){
               .font = FONT_ICONS_XL,
               .label = fromconst(ICON_PLAY),
               .onclick = audio_play,
               .background = theme->colors[COLOR_BARBG],
               .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
           });
  } else {
    ui_btn(winw / 2 - primarybtnw / 2, y + height / 2 - primarybtnh / 2,
           primarybtnw, primarybtnh,
           (btndesc_t){
               .font = FONT_ICONS_XL,
               .label = fromconst(ICON_PAUSE),
               .onclick = audio_pause,
               .background = theme->colors[COLOR_BARBG],
               .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
           });
  }

  ui_btn(winw / 2 + primarybtnw / 2 + spacebetween,
         y + height / 2 - secondarybtnh / 2, secondarybtnw, secondarybtnh,
         (btndesc_t){
             .font = FONT_ICONS_L,
             .label = fromconst(ICON_LOOP),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
         });

  ui_btn(winw / 2 - primarybtnw / 2 - secondarybtnw - spacebetween,
         y + height / 2 - secondarybtnh / 2, secondarybtnw, secondarybtnh,
         (btndesc_t){
             .font = FONT_ICONS_L,
             .label = fromconst(ICON_STOP),
             .onclick = audio_stop,
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
         });
}
