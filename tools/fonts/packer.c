#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  NBITMAP = 512,
  NBITMAP2 = NBITMAP * NBITMAP,
  NBUF = 1 << 20,
};
unsigned char fontbuf[NBUF];
unsigned char bitmap[NBITMAP2];
stbtt_bakedchar chardata[96];

#define len(arr) (sizeof(arr) / sizeof((arr)[0]))

// strndup is not available on all platforms
static inline char *dupe(const char *str, unsigned len) {
  char *buf = malloc(len + 1);
  memcpy(buf, str, len);
  buf[len] = '\0';
  return buf;
}

const char *basename(const char *path) {
  for (unsigned i = strlen(path); i >= 0; i--) {
    if (path[i] == '/')
      return &path[i + 1];
  }

  return path;
}

char *filename(const char *basename) {
  for (unsigned i = strlen(basename); i >= 0; i--) {
    if (basename[i] == '.') {
      return dupe(basename, i);
    }
  }
  return strdup(basename);
}

// Writes a .h (extern declarations) and .c (definitions) pair for a font.
void writefile(const char *dir, const char *file, float pts, float nfont,
               const char *suffix) {
  memset(bitmap, 0, sizeof(bitmap));
  stbtt_BakeFontBitmap(fontbuf, 0, nfont, bitmap, NBITMAP, NBITMAP, 32, 96,
                       chardata);
  // This solid dot will be used to render rectangles.
  // Prevents adding a second pipeline
  bitmap[NBITMAP2 - 1] = 0xFF;

  unsigned char *asbuf = (unsigned char *)chardata;
  size_t bufsz = sizeof(chardata);

  char hpath[256];
  char cpath[256];
  snprintf(hpath, sizeof(hpath), "%s/%s%.0f%s.h", dir, file, pts, suffix);
  snprintf(cpath, sizeof(cpath), "%s/%s%.0f%s.c", dir, file, pts, suffix);

  printf("Packing font at %s...\n", hpath);

  FILE *hfile = fopen(hpath, "w");
  assert(hfile && "writefile: Unable to open header file");
  fprintf(hfile,
          "#pragma once\n\n"
          "extern _Alignas(4) unsigned char baked%s%s%.0f[%lu];\n"
          "extern unsigned char atlas%s%s%.0f[%d];\n",
          file, suffix, pts, bufsz, file, suffix, pts, NBITMAP2);
  fclose(hfile);

  FILE *cfile = fopen(cpath, "w");
  assert(cfile && "writefile: Unable to open source file");
  fprintf(cfile, "#include \"%s%.0f%s.h\"\n\n", file, pts, suffix);

  fprintf(cfile, "_Alignas(4) unsigned char baked%s%s%.0f[%lu] = {", file,
          suffix, pts, bufsz);
  for (size_t i = 0; i < bufsz; i++) {
    if (i % 12 == 0) {
      fprintf(cfile, "\n ");
    }
    fprintf(cfile, " 0x%02x,", asbuf[i]);
  }
  fprintf(cfile, "\n};\n\n");

  fprintf(cfile, "unsigned char atlas%s%s%.0f[%d] = {", file, suffix, pts,
          NBITMAP2);
  for (int i = 0; i < NBITMAP2; i++) {
    if (i % 12 == 0) {
      fprintf(cfile, "\n ");
    }
    fprintf(cfile, " 0x%02x,", bitmap[i]);
  }
  fprintf(cfile, "\n};\n");
  fclose(cfile);

  printf("Packing font at %s... OK\n", hpath);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Packs TrueTypeFont into a bitmap and saves it to disk.\n");
    fprintf(stderr, "Size is in points (at 72dpi) as in design tools.\n\n");
    fprintf(stderr, "usage: %s [font file] [font size in points]\n", argv[0]);
    fprintf(stderr, "       %s font.ttf 12\n", argv[0]);
    exit(1);
  }

  const char *fontpath = argv[1];
  float points = (float)atof(argv[2]);
  assert(!isnan(points) && "main: Unable to parse font size");

  // Figma uses points at 72 DPI; convert to pixels at 96 DPI
  float nfont = points * (96.0F / 72.0F);
  float nfont2x = nfont * 2.0F;

  char *file = filename(basename(fontpath));
  const char *dir = "src/gfx/fonts";

  FILE *fontfile = fopen(fontpath, "r");
  assert(fontfile && "main: Unable to open font file");

  size_t offset = fread(fontbuf, 1, NBUF, fontfile);
  assert(offset > 0 && "main: Unable to read opened font file");
  fclose(fontfile);

  puts("Baking font into bitmap...");
  writefile(dir, file, points, nfont, "");
  puts("Baking font into bitmap... OK");

  puts("Baking hi-res font into bitmap...");
  writefile(dir, file, points, nfont2x, "hidpi");
  puts("Baking hi-res font into bitmap... OK");

  return 0;
}
