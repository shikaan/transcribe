#pragma once

#include <sokol_app.h>
#include <stdbool.h>

typedef enum {
  MOUSEBTNL = SAPP_MOUSEBUTTON_LEFT,
  MOUSEBTNR = SAPP_MOUSEBUTTON_RIGHT,
  MOUSEBTNM = SAPP_MOUSEBUTTON_MIDDLE,
  EMOUSEBTN = SAPP_MOUSEBUTTON_INVALID,
} mousebtn_t;

typedef struct {
  float mousex;
  float mousey;
  float scrollx;
  float scrolly;
  mousebtn_t mousebtn;
  bool clicked : 1;
  bool pressed : 1;
  bool down : 1;
  bool up : 1;
} input_t;

extern input_t input;

void input_event(const sapp_event *ev);
void input_done(void);
