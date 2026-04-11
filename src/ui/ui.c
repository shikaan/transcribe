#include "ui.h"

#include "ui/btn.c"
#include "ui/num.c"
#include "ui/tick.c"

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
// ---
// These constants are elements we will turn into configuration once
// configuration exists.
static const float SCROLL_FACTOR = 16;
static const float SCROLLX_DIRECTION = -1;
static const float SAMPLES_PER_WAVEBAR = 8;
static const bool FOLLOW_PLAYHEAD = true;

// Constants
// ---
// Constants that are hoisted to be used by more than one function.
static const float MIN_ZOOM = 0.5;
static const float MAX_ZOOM = 4;

// UI Spacing Constants
// ---
// Shared spacing constants. They are meant to be used like
//   theme->spacing * $VALUE
static const float TITLEBARH = 6;
static const float TIMELINEH = 4;
static const float SPECTRUMH = 32;
static const float BARPADX = 3;
static const float WAVEBARW = 0.5;
static const float WAVEBARGAP = 0.25;

// UI State
// ---
// Shared UI state. Each element of this list affects multiple components.
// Handle with care.
static char fmtbuf[16] = {0};
static bool isempty = true;
static bool isplaying = false;
static bool islooping = false;
static float winw = 0;
static float winh = 0;
static float totalsec = 0;
static float currentsec = 0;
static float scrollx = 0;
static float zoom = 1.0f;
static samples_t *samples = 0;
static float loopa = 0;
static float loopb = 0;

#ifndef NDEBUG
// This block exists uniquely not to break the UI when we load the app in debug
// mode with a preloaded file.
static char namebuf[1024] = "test.mp3";
static string_t filename = {namebuf, 8};
#else
static char namebuf[1024] = {0};
static string_t filename = {namebuf, 0};
#endif

static void openfile(void) {
  char pathbuf[1024] = {0};
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

// Recalculate scrollx. To be used after a zoom or a scroll event.
static void scroll(float delta, float direction) {
  const float barpx = theme->spacing * (WAVEBARW + WAVEBARGAP);
  const float tracklen =
      (float)samples->len * barpx * zoom / SAMPLES_PER_WAVEBAR;
  const float newscrollx = scrollx + direction * delta * SCROLL_FACTOR;
  scrollx = clamp(newscrollx, 0, tracklen - winw);
}

// Zooms in and recalculates scroll position
static void zoomin(void) {
  zoom = clamp(zoom * 2, MIN_ZOOM, MAX_ZOOM);
  scroll(input.scrollx, SCROLLX_DIRECTION);
}

// Zooms out and recalculates scroll position
static void zoomout(void) {
  zoom = clamp(zoom / 2, MIN_ZOOM, MAX_ZOOM);
  scroll(input.scrollx, SCROLLX_DIRECTION);
}

static void ui_zoom(float x, float y, float w, float h) {
  const float btnh = theme->spacing * 4;
  const float btnw = btnh;

  ui_btn(x, y + h / 2 - btnh / 2, btnw, btnh,
         (btndesc_t){
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTLIGHT],
             .icon = string(ICON_ZOOMOUT),
             .iconfont = FONT_ICONS_M,
             .onclick = zoomout,
         });

  string_t str = {fmtbuf, fmt_zoom(zoom, fmtbuf, sizeof(fmtbuf))};

  textdesc_t desc = {FONT_MONO_M, 0};
  float txtw, txth;
  gfx_textbox(str, desc, &txtw, &txth);
  gfx_text(x + w / 2 - txtw / 2, h / 2 + txth / 2, str, desc,
           theme->colors[COLOR_TEXTLIGHT]);

  ui_btn(x + w - btnw, y + h / 2 - btnh / 2, btnw, btnh,
         (btndesc_t){
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTLIGHT],
             .icon = string(ICON_ZOOMIN),
             .iconfont = FONT_ICONS_M,
             .onclick = zoomin,
         });
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
  textdesc_t desc = {FONT_ICONS_M, 0};
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

  const float zoomw = theme->spacing * 14;
  ui_zoom(winw - padx - zoomw, height / 2 - height / 2, zoomw, height);

  gfx_line(0, height - 1, winw, LINEDIR_H, theme->colors[COLOR_TEXTDIM]);
}

static void ui_waveform(void) {
  // Waveform
  const float wavey = theme->spacing * (TITLEBARH + TIMELINEH);
  const float wavex = 0;
  const float waveh = theme->spacing * SPECTRUMH;
  const float barw = theme->spacing * WAVEBARW;
  const float barminh = theme->spacing / 2;
  const float barmaxh = waveh - theme->spacing - barminh;
  const float bargap = theme->spacing * WAVEBARGAP;
  const float barpx = barw + bargap;
  const float tracklen =
      (float)samples->len * barpx * zoom / SAMPLES_PER_WAVEBAR;
  const size_t samplestride = (size_t)(SAMPLES_PER_WAVEBAR / zoom);

  // Playhead
  const float cursorw = theme->spacing;
  const float thickness = theme->spacing / 4;
  const float deadzone = theme->spacing * 20;

  // Timeline
  const float sectopx = tracklen / totalsec;
  const float pxtosec = totalsec / tracklen;
  const float timeliney = theme->spacing * TITLEBARH;
  const float timelineh = theme->spacing * TIMELINEH;
  const float lineh = theme->spacing;
  const float llineh = theme->spacing * 1.5f;
  const float timepad = theme->spacing / 4;
  const float tlsecperstride = 1 / zoom;

  scroll(input.scrollx, SCROLLX_DIRECTION);

  gfx_rect(wavex, wavey, winw, waveh, theme->colors[COLOR_SPECTRUMBG]);

  // Loop region
  if (islooping) {
    float regx = loopa * sectopx - scrollx;
    float regy = wavey;
    float regw = loopb * sectopx - scrollx - regx;
    float regh = waveh;
    gfx_rect(regx, regy, regw, regh, theme->colors[COLOR_SELECTION]);
  }

  const float waveoffset = fmodf(scrollx, barpx);
  size_t sample = (size_t)(scrollx / barpx) * samplestride;
  float barx = -waveoffset;
  for (; barx < winw && sample < samples->len;
       barx += barpx, sample += samplestride) {
    float barh = barmaxh * samples->buf[sample] + barminh;
    float bary = wavey + (waveh - barh) / 2;
    gfx_rect(barx, bary, barw, barh, theme->colors[COLOR_TEXTLIGHT]);
  }

  // Playhead
  float headx = currentsec * sectopx - scrollx;
  if (overlaps(wavex, wavey, winw, waveh, input.mousex, input.mousey)) {
    if (input.pressed) {
      audio_seek((input.mousex + scrollx) * pxtosec);
      headx = input.mousex;
    }

    const float minloopsize = sectopx / 60;
    const bool dragging = (input.down && !input.pressed);
    const bool done = (input.up && input.dragstartx > 0);

    if (dragging || done) {
      float selx = fminf(input.dragstartx, input.mousex);
      float selw = fabsf(input.mousex - input.dragstartx);
      gfx_rect(selx, wavey, selw, waveh, theme->colors[COLOR_SELECTION]);
    }

    if (done) {
      float dx = fabsf(input.mousex - input.dragstartx);
      if (dx > minloopsize) {
        float a = (input.dragstartx + scrollx) * pxtosec;
        float b = (input.mousex + scrollx) * pxtosec;
        if (a > b) {
          float t = a;
          a = b;
          b = t;
        }
        audio_setloopregion(a, b);
        audio_seek(a);
        audio_setloop(true);
      }
    }
  }

  if (FOLLOW_PLAYHEAD && isplaying) {
    // Using fixed scroll direction on purpose here: this direction does NOT
    // change with users' preference.

    // Scroll to the playhead when it's before beginning of the screen
    if (headx < 0) {
      scroll(-headx, -1);
    }

    if (headx > winw - deadzone) {
      scroll(-SCROLL_FACTOR, -1);
    }
  }

  if (headx < winw && headx >= 0) {
    gfx_rect(headx - cursorw / 2 + thickness / 2, wavey, cursorw, cursorw,
             theme->colors[COLOR_PRIMARY]);
    gfx_rect(headx, wavey, thickness, waveh, theme->colors[COLOR_PRIMARY]);
  }

  // Timeline
  string_t timecode = {fmtbuf, 0};
  float tlstride = sectopx * tlsecperstride;
  float timey = timeliney + timelineh - 1 - llineh;
  float tloffset = fmodf(scrollx, tlstride);
  float seconds = (scrollx - tloffset) / tlstride * tlsecperstride;

  gfx_rect(0, timeliney, winw, timelineh, theme->colors[COLOR_BARBG]);
  gfx_line(0, timeliney + timelineh - 1, winw, LINEDIR_H,
           theme->colors[COLOR_TEXTDIM]);

  textdesc_t desc = {FONT_MONO_S, 0};
  for (float timex = -tloffset; timex < winw; timex += tlstride) {
    timecode.len =
        fmt_sectotime(seconds, FMT_TIMECODESHORT, fmtbuf, len(fmtbuf));
    gfx_text(timex + 1, timey - timepad, timecode, desc,
             theme->colors[COLOR_TEXTLIGHT]);
    gfx_line(timex, timey, llineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
    seconds += tlsecperstride;
  }

  timey = timeliney + timelineh - 1 - lineh;
  float half = tlstride / 2;
  tloffset = fmodf(scrollx - half, tlstride);
  float linex = -tloffset;
  for (; linex < winw; linex += tlstride) {
    gfx_line(linex, timey, lineh, LINEDIR_V, theme->colors[COLOR_TEXTDIM]);
  }
}

static void stop(void) {
  audio_stop();
  audio_setloop(false);
  if (FOLLOW_PLAYHEAD)
    scrollx = 0;
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
  string_t timecode = {
      fmtbuf,
      fmt_sectotime(currentsec, FMT_TIMECODE, fmtbuf, len(fmtbuf)),
  };
  currentx += padx;
  textdesc_t desc = {FONT_MONO_L, 0};
  gfx_textbox(timecode, desc, &txtw, &txth);
  gfx_text(currentx, y + height / 2 + txth / 2, timecode, desc,
           theme->colors[COLOR_TEXTHIGHLIGHT]);

  timecode.len = fmt_sectotime(totalsec, FMT_TIMECODE, fmtbuf, len(fmtbuf));
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

  ui_tick(loopx, secbtny, secbtnw, secbtnh,
          (tickdesc_t){.iconfont = FONT_ICONS_L,
                       .icon = string(ICON_LOOP),
                       .value = islooping,
                       .background = {theme->colors[COLOR_BARBG],
                                      theme->colors[COLOR_PRIMARY]},
                       .foreground = {theme->colors[COLOR_TEXTHIGHLIGHT],
                                      theme->colors[COLOR_BARBG]},
                       .onchange = audio_setloop});

  ui_btn(stopx, secbtny, secbtnw, secbtnh,
         (btndesc_t){
             .iconfont = FONT_ICONS_L,
             .icon = string(ICON_STOP),
             .background = theme->colors[COLOR_BARBG],
             .foreground = theme->colors[COLOR_TEXTHIGHLIGHT],
             .onclick = stop,
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
  // winw and winh need to be calculated on every frame to account for resizing
  const float scale = sapp_dpi_scale();
  winw = sapp_widthf() / scale;
  winh = sapp_heightf() / scale;
  audio_isplaying(&isplaying);
  audio_islooping(&islooping);
  audio_times(&totalsec, &currentsec);
  audio_samples(&samples);
  isempty = totalsec <= 0;
  audio_loopregion(&loopa, &loopb);

  if (isempty) {
    ui_emptystate();
  } else {
    ui_titlebar();
    ui_waveform();
    ui_controls();
  }
}
