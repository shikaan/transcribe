#pragma once

#include "types.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <tinyfiledialogs.h>

static const char *ALLOWED_FILES[] = {"*.mp3"};

static inline result_t files_open(char *filepath, size_t maxlen) {
  assert(filepath && "files_open: filepath cannot be null");

  char *path = tinyfd_openFileDialog("Open file...", 0, len(ALLOWED_FILES),
                                     ALLOWED_FILES, "Audio file", 0);
  if (!path) {
    return ERR_FILES_CANCEL;
  }

  snprintf(filepath, maxlen, "%s", path);
  return OK;
}
