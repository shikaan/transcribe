#include "input.h"
#include <stdio.h>

input_t input = {0};

void input_event(const sapp_event *ev) {
  float scale = sapp_dpi_scale();

  switch (ev->type) {
  case SAPP_EVENTTYPE_MOUSE_DOWN:
    input.mousebtn = (mousebtn_t)ev->mouse_button;
    input.down = 1;
    input.pressed = 1;
    input.up = 0;
    break;
  case SAPP_EVENTTYPE_MOUSE_UP:
    input.mousebtn = (mousebtn_t)ev->mouse_button;
    input.clicked = input.down;
    input.up = 1;
    input.down = 0;
    break;
  case SAPP_EVENTTYPE_MOUSE_MOVE:
    input.mousex = ev->mouse_x / scale;
    input.mousey = ev->mouse_y / scale;
    break;
  case SAPP_EVENTTYPE_MOUSE_SCROLL:
    input.scrollx = ev->scroll_x;
    input.scrolly = ev->scroll_y;
    break;
  case SAPP_EVENTTYPE_MOUSE_ENTER:
  case SAPP_EVENTTYPE_MOUSE_LEAVE:
  case SAPP_EVENTTYPE_INVALID:
  case SAPP_EVENTTYPE_KEY_DOWN:
  case SAPP_EVENTTYPE_KEY_UP:
  case SAPP_EVENTTYPE_CHAR:
  case SAPP_EVENTTYPE_TOUCHES_BEGAN:
  case SAPP_EVENTTYPE_TOUCHES_MOVED:
  case SAPP_EVENTTYPE_TOUCHES_ENDED:
  case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
  case SAPP_EVENTTYPE_RESIZED:
  case SAPP_EVENTTYPE_ICONIFIED:
  case SAPP_EVENTTYPE_RESTORED:
  case SAPP_EVENTTYPE_FOCUSED:
  case SAPP_EVENTTYPE_UNFOCUSED:
  case SAPP_EVENTTYPE_SUSPENDED:
  case SAPP_EVENTTYPE_RESUMED:
  case SAPP_EVENTTYPE_QUIT_REQUESTED:
  case SAPP_EVENTTYPE_CLIPBOARD_PASTED:
  case SAPP_EVENTTYPE_FILES_DROPPED:
  case _SAPP_EVENTTYPE_NUM:
  case _SAPP_EVENTTYPE_FORCE_U32:
  default:
    break;
  }
}

void input_done(void) {
  input.clicked = 0;
  input.pressed = 0;
  input.scrollx = 0;
  input.scrolly = 0;
}
