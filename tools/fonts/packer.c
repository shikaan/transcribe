#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char fontbuf[1 << 20];
unsigned char bitmap[512 * 512];
stbtt_bakedchar chardata[96];

#define len(arr) sizeof(arr) / sizeof(arr[0])

const char *basename(const char *path) {
  for (int i = strlen(path); i >= 0; i--) {
    if (path[i] == '/')
      return &path[i + 1];
  }

  return path;
}

char *filename(const char *basename) {
  for (int i = strlen(basename); i >= 0; i--) {
    if (basename[i] == '.') {
      return strndup(basename, i);
    }
  }
  return strdup(basename);
}

char *dirname(const char *path) {
  for (int i = strlen(path); i >= 0; i--) {
    if (path[i] == '/') {
      return strndup(path, i);
    }
  }
  return ".";
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Packs TrueTypeFont into a bitmap and saves it to disk.\n");
    fprintf(stderr, "usage: %s [FONT FILE] [FONT SIZE]\n", argv[0]);
    fprintf(stderr, "       %s font.ttf 16.0\n", argv[0]);
    exit(1);
  }

  const char *fontpath = argv[1];
  char outpath[256] = {0};
  char *file = filename(basename(fontpath));
  snprintf(outpath, 256, "%s/%s.h", dirname(fontpath), file);

  float fontsz = atof(argv[2]);
  assert(!isnan(fontsz) && "main: Unable to parse font size");

  FILE *fontfile = fopen(fontpath, "r");
  assert(fontfile && "main: Unable to open font file");

  size_t sz = fread(fontbuf, 1, 1 << 20, fontfile);
  assert(sz > 0 && "main: Unable to read opened font file");

  stbtt_BakeFontBitmap(fontbuf, 0, fontsz, bitmap, 512, 512, 32, 96, chardata);

  FILE *outputfile = fopen(outpath, "w");
  assert(outputfile && "main: Unable to open outputfile");

  unsigned char *asbuf = (unsigned char *)chardata;
  size_t bufsz = sizeof(chardata);

  fprintf(outputfile, "#pragma once\nunsigned char %s[] = {", file);
  for (int i = 0; i < bufsz; i++) {
    if (i % 12 == 0) {
      fprintf(outputfile, "\n ");
    }
    unsigned char byte = asbuf[i];
    fprintf(outputfile, " 0x%02x,", byte);
  }
  fprintf(outputfile, "\n};\nunsigned int n%s = %lu;\n", file, bufsz);

  return 0;
}
