// UNTESTED
#include "../types.h"
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

result_t files_open(char *filepath, size_t maxlen) {
  static bool opening = false;
  if (opening)
    return ERR_FILES_CANCEL;
  opening = true;

  char buf[1024] = {0};
  OPENFILENAMEA ofn = {0};
  ofn.lStructSize = sizeof(ofn);
  ofn.lpstrFile = buf;
  ofn.nMaxFile = sizeof(buf);
  ofn.lpstrFilter = "Audio Files\0*.mp3\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  result_t result = ERR_FILES_CANCEL;
  if (GetOpenFileNameA(&ofn)) {
    snprintf(filepath, maxlen, "%s", buf);
    result = OK;
  }

  opening = false;
  return result;
}
