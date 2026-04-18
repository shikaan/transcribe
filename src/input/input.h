#pragma once

#include <sokol_app.h>
#include <stdbool.h>

typedef enum {
  MOUSEBTN_INVALID = SAPP_MOUSEBUTTON_INVALID,
  MOUSEBTN_LEFT = SAPP_MOUSEBUTTON_LEFT,
  MOUSEBTN_RIGHT = SAPP_MOUSEBUTTON_RIGHT,
  MOUSEBTN_MIDDLE = SAPP_MOUSEBUTTON_MIDDLE,
} mousebtn_t;

typedef enum {
  CURSOR_DEFAULT = SAPP_MOUSECURSOR_DEFAULT,
  CURSOR_POINTER = SAPP_MOUSECURSOR_POINTING_HAND,
} cursor_t;

typedef struct {
  float mousex;
  float mousey;
  float scrollx;
  float scrolly;
  float dragstartx;
  float dragstarty;
  mousebtn_t mousebtn;
  bool clicked : 1;
  bool pressed : 1;
  bool down : 1;
  bool up : 1;
} input_t;

extern input_t input;

void input_event(const sapp_event *ev);
void input_done(void);
void input_setcursor(cursor_t c);

static inline bool input_lclicked(void) {
  return input.clicked && input.mousebtn == MOUSEBTN_LEFT;
}
