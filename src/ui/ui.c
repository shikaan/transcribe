#include "ui.h"

#include "ui/btn.c"
#include "ui/num.c"

#include "audio/audio.h"
#include "files/files.h"
#include "fmt/fmt.h"
#include "gfx/gfx.h"
#include "input/input.h"
#include "theme.h"
#include "types.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <sokol_app.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Configuration
static const float SCROLL_FACTOR = 16;
static const float SCROLL_DIRECTION = -1;

// UI Constants
static const float TITLEBARH = 6;
static const float TIMELINEH = 4;
static const float SPECTRUM_H = 32;
static const float BARPADX = 3;

// UI State
static char timebuf[16] = "";
static bool isempty = true;
static bool isplaying = false;
static float winw = 0;
static float winh = 0;
static float totalsec = 0;
static float currentsec = 0;
static float scrollx = 0;

static char namebuf[1024] = {0};
static string_t filename = {namebuf, 0};
static char pathbuf[1024] = {0};

static void openfile(void) {
  result_t r = files_open(pathbuf, len(pathbuf));
  // TODO: tell user why the file was not loaded (e.g., "Operation cancelled")
  if (r == ERR_FILES_CANCEL) {
    return;
  }

  r = audio_open(pathbuf);
  // TODO: error handling
  if (r != OK) {
    sapp_request_quit();
    return;
  }

  filename.len = fmt_basename(pathbuf, namebuf, sizeof(namebuf));
  scrollx = 0;
}

static void ui_titlebar(void) {
  const float height = theme->spacing * TITLEBARH;
  const float padx = theme->spacing * BARPADX;
  const float iconspace = theme->spacing;
  const float spacebetween = theme->spacing * 2;
  const float sepheight = theme->spacing * 2;
  float txtw = 0, txth = 0;

  gfx_rect(0, 0, winw, height, theme->colors[COLOR_BARBG]);

  float currentx = 0;
  string_t puzzle = string(ICON_PUZZLE);
  textdesc_t desc = (textdesc_t){FONT_ICONS_M, 0};
  gfx_textbox(puzzle, desc, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, puzzle, desc,
           theme->colors[COLOR_PRIMARY]);

  string_t logo = string("Transcribling");
  currentx += iconspace + txtw;
  desc.font = FONT_SANS_BOLD_M;
  gfx_textbox(logo, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, logo, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  currentx += txtw + padx;
  gfx_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
           theme->colors[COLOR_TEXTDIM]);

  const float btnw = theme->spacing * 10;
  const float btnh = theme->spacing * 4;

  currentx += iconspace;
  ui_btn(currentx, height / 2 - btnh / 2, btnw, btnh,
         (btndesc_t){
             .icon = string(ICON_OPEN),
             .iconfont = FONT_ICONS_M,
             .label = string("Open"),
             .labelfont = FONT_SANS_M,
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTLIGHT],
             .onclick = openfile,
         });

  currentx += btnw + iconspace;
  gfx_line(currentx, height / 2 - sepheight / 2, sepheight, LINEDIR_V,
           theme->colors[COLOR_TEXTDIM]);

  string_t fileicon = string(ICON_FILE);
  desc.font = FONT_ICONS_M;
  gfx_textbox(fileicon, desc, &txtw, &txth);
  currentx += spacebetween;
  gfx_text(currentx, height / 2 + txth / 2, fileicon, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  currentx += iconspace + txtw;
  desc.font = FONT_SANS_M;
  gfx_textbox(logo, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, filename, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  string_t settingsicon = string(ICON_EQ);
  desc.font = FONT_ICONS_M;
  gfx_textbox(settingsicon, desc, &txtw, &txth);
  gfx_text(winw - txtw - padx, height / 2 + txth / 2, settingsicon, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  gfx_line(0, height - 1, winw, LINEDIR_H, theme->colors[COLOR_TEXTDIM]);
}

static void ui_waveform(void) {
  samples_t *samples;
  audio_samples(&samples);

  // Waveform
  const float wavey = theme->spacing * (TITLEBARH + TIMELINEH);
  const float wavex = 0;
  const float waveh = theme->spacing * SPECTRUM_H;
  const float barw = theme->spacing / 2;
  const float barminh = theme->spacing / 2;
  const float barmaxh = waveh - theme->spacing - barminh;
  const float bargap = theme->spacing / 4;
  const float tracklen = (float)samples->len * (barw + bargap);

  // Playhead
  const float heady = theme->spacing * (TITLEBARH + TIMELINEH);
  const float cursorw = theme->spacing;
  const float thickness = theme->spacing / 4;

  // Timeline
  const float sectopx = tracklen / totalsec;
  const float timeliney = theme->spacing * TITLEBARH;
  const float timelineh = theme->spacing * TIMELINEH;
  const float lineh = theme->spacing;
  const float linew = 1;
  const float llineh = theme->spacing * 1.5f;
  const float timepad = theme->spacing / 4;
  const float secperstride = 1;

  float newscrollx = scrollx + SCROLL_DIRECTION * input.scrollx * SCROLL_FACTOR;
  scrollx = clamp(newscrollx, 0, tracklen - winw);

  // Waveform
  gfx_rect(wavex, wavey, winw, waveh, theme->colors[COLOR_SPECTRUMBG]);
  size_t sample = (size_t)(scrollx / (barw + bargap));
  for (float x = wavex; x < winw && sample < samples->len;
       x += barw + bargap, sample++) {
    float barh = barmaxh * samples->buf[sample] + barminh;
    gfx_rect(x, wavey + (waveh - barh) / 2, barw, barh,
             theme->colors[COLOR_TEXTLIGHT]);
  }

  // Playhead
  float headx = (tracklen * currentsec / totalsec) - scrollx;
  if (input.pressed &&
      overlaps(wavex, wavey, winw, waveh, input.mousex, input.mousey)) {
    audio_seek((input.mousex + scrollx) * totalsec / tracklen);
    headx = input.mousex;
  }

  if (headx < winw) {
    gfx_rect(headx - cursorw / 2 + thickness / 2, heady, cursorw, cursorw,
             theme->colors[COLOR_PRIMARY]);
    gfx_rect(headx, heady, thickness, waveh, theme->colors[COLOR_PRIMARY]);
  }

  // Timeline
  string_t time = {timebuf, 5};
  float stride = sectopx * secperstride;
  float timey = timeliney + timelineh - 1 - llineh;
  float offset = fmodf(scrollx, stride); // How many strides in scroll?
  float seconds = (scrollx - offset) / stride * secperstride;

  gfx_rect(0, timeliney, winw, timelineh, theme->colors[COLOR_BARBG]);
  gfx_line(0, timeliney + timelineh - 1, winw, LINEDIR_H,
           theme->colors[COLOR_TEXTDIM]);

  textdesc_t desc = {FONT_MONO_S, 0};
  for (float timex = -offset; timex < winw; timex += stride) {
    time.len = fmt_sectotime(seconds, FMT_TIMECODESHORT, timebuf, len(timebuf));
    gfx_text(timex + linew, timey - timepad, time, desc,
             theme->colors[COLOR_TEXTLIGHT]);
    gfx_line(timex, timey, llineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
    seconds += secperstride;
  }

  timey = timeliney + timelineh - 1 - lineh;
  float half = stride / 2;
  offset = fmodf(scrollx - half, stride);
  float linex = -offset;
  for (; linex < winw; linex += stride) {
    gfx_line(linex, timey, lineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
  }
}

static void ui_controls(void) {
  isempty = totalsec <= 0;

  const float height = theme->spacing * 12;
  const float padx = theme->spacing * BARPADX;
  const float y = winh - height;
  const float primbtnw = theme->spacing * 8;
  const float primbtnh = theme->spacing * 8;
  const float primbtnx = winw / 2 - primbtnw / 2;
  const float primbtny = y + height / 2 - primbtnh / 2;
  const float secbtnw = theme->spacing * 6;
  const float secbtnh = theme->spacing * 6;
  const float secbtny = y + height / 2 - secbtnh / 2;
  const float numberh = theme->spacing * 5;
  const float numberw = theme->spacing * 14;

  float txtw = 0, txth = 0, currentx = 0, spacebetween = 0;
  gfx_rect(0, y, winw, height, theme->colors[COLOR_BARBG]);

  // Timecode
  string_t timecode = {timebuf, 10};

  timecode.len = fmt_sectotime(currentsec, FMT_TIMECODE, timebuf, len(timebuf));
  currentx += padx;
  textdesc_t desc = {FONT_MONO_L, 0};
  gfx_textbox(timecode, desc, &txtw, &txth);
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  timecode.len = fmt_sectotime(totalsec, FMT_TIMECODE, timebuf, len(timebuf));
  currentx += txtw + theme->spacing * 2;
  desc.font = FONT_MONO_M;
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  // Inputs
  spacebetween = theme->spacing * 4;

  currentx = winw - padx - numberw;
  ui_num(currentx, winh - (height + numberh) / 2, numberw, numberh,
         string("PITCH"), string("0 st"));

  currentx -= numberw + spacebetween;
  ui_num(currentx, winh - (height + numberh) / 2, numberw, numberh,
         string("TEMPO"), string("100%"));

  // Buttons
  spacebetween = theme->spacing * 2;
  const float loopx = winw / 2 + primbtnw / 2 + spacebetween;
  const float stopx = winw / 2 - primbtnw / 2 - secbtnw - spacebetween;

  ui_btn(primbtnx, primbtny, primbtnw, primbtnh,
         (btndesc_t){
             .iconfont = FONT_ICONS_XL,
             .icon = isplaying ? string(ICON_PAUSE) : string(ICON_PLAY),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
             .onclick = isplaying ? audio_pause : audio_play,
         });

  ui_btn(loopx, secbtny, secbtnw, secbtnh,
         (btndesc_t){
             .iconfont = FONT_ICONS_L,
             .icon = string(ICON_LOOP),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
         });

  ui_btn(stopx, secbtny, secbtnw, secbtnh,
         (btndesc_t){
             .iconfont = FONT_ICONS_L,
             .icon = string(ICON_STOP),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
             .onclick = audio_stop,
         });
}

static void ui_emptystate(void) {
  const float height = theme->spacing * TITLEBARH;
  const float padx = theme->spacing * BARPADX;
  const float iconspace = theme->spacing;
  const float btnw = theme->spacing * 20;
  const float btnh = theme->spacing * 5;

  float txtw = 0, txth = 0;

  gfx_rect(0, 0, winw, height, theme->colors[COLOR_BARBG]);

  float currentx = 0;
  string_t puzzle = string(ICON_PUZZLE);
  textdesc_t desc = (textdesc_t){FONT_ICONS_M, 0};
  gfx_textbox(puzzle, desc, &txtw, &txth);
  currentx += padx;
  gfx_text(currentx, height / 2 + txth / 2, puzzle, desc,
           theme->colors[COLOR_PRIMARY]);

  string_t logo = string("Transcribling");
  currentx += iconspace + txtw;
  desc.font = FONT_SANS_BOLD_M;
  gfx_textbox(logo, desc, &txtw, &txth);
  gfx_text(currentx, height / 2 + txth / 2, logo, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  ui_btn(winw / 2 - btnw / 2, winh / 2 - btnh / 2, btnw, btnh,
         (btndesc_t){
             .icon = string(ICON_OPEN),
             .iconfont = FONT_ICONS_M,
             .label = string("Open audio file"),
             .labelfont = FONT_SANS_M,
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
             .onclick = openfile,
         });
}

void ui_app(void) {
  const float scale = sapp_dpi_scale();
  winw = sapp_widthf() / scale;
  winh = sapp_heightf() / scale;
  audio_isplaying(&isplaying);
  audio_times(&totalsec, &currentsec);
  isempty = totalsec <= 0;

  if (isempty) {
    ui_emptystate();
  } else {
    ui_titlebar();
    ui_waveform();
    ui_controls();
  }
}
