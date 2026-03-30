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

// Writes a file on disk with the same output shape as xxd -i
void writefile(const char *outpath, const char *file, float pts, float nfont) {
  memset(bitmap, 0, sizeof(bitmap));
  stbtt_BakeFontBitmap(fontbuf, 0, nfont, bitmap, NBITMAP, NBITMAP, 32, 96,
                       chardata);
  // This solid dot will be used to render rectangles.
  // Prevents adding a second pipeline
  bitmap[NBITMAP2 - 1] = 0xFF;

  FILE *outputfile = fopen(outpath, "w");
  assert(outputfile && "writefile: Unable to open outputfile");

  unsigned char *asbuf = (unsigned char *)chardata;
  size_t bufsz = sizeof(chardata);

  printf("Packing font at %s...\n", outpath);
  fprintf(outputfile,
          "#pragma once\n"
          "_Alignas(4) unsigned char baked%s%.0f[%lu] = {",
          file, pts, bufsz);
  for (int i = 0; i < bufsz; i++) {
    if (i % 12 == 0) {
      fprintf(outputfile, "\n ");
    }
    fprintf(outputfile, " 0x%02x,", asbuf[i]);
  }
  fprintf(outputfile, "\n};\n");

  fprintf(outputfile, "unsigned char atlas%s%.0f[%d] = {", file, pts, NBITMAP2);
  for (int i = 0; i < len(bitmap); i++) {
    if (i % 12 == 0) {
      fprintf(outputfile, "\n ");
    }
    fprintf(outputfile, " 0x%02x,", bitmap[i]);
  }
  fprintf(outputfile, "\n};\n");
  printf("Packing font at %s... OK\n", outpath);

  fclose(outputfile);
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
  float points = atof(argv[2]);
  assert(!isnan(points) && "main: Unable to parse font size");

  // Figma uses points at 72 DPI; convert to pixels at 96 DPI
  float nfont = points * (96.0f / 72.0f);
  float nfont2x = nfont * 2.0f;

  char *file = filename(basename(fontpath));
  char *dir = "src/gfx/fonts";

  FILE *fontfile = fopen(fontpath, "r");
  assert(fontfile && "main: Unable to open font file");

  size_t offset = fread(fontbuf, 1, NBUF, fontfile);
  assert(offset > 0 && "main: Unable to read opened font file");
  fclose(fontfile);

  char outpath[256];

  puts("Baking font into bitmap...");
  snprintf(outpath, sizeof(outpath), "%s/%s.%.0f.h", dir, file, points);
  writefile(outpath, file, points, nfont);
  puts("Baking font into bitmap... OK");

  puts("Baking hi-res font into bitmap...");
  snprintf(outpath, sizeof(outpath), "%s/%s.%.0f@2x.h", dir, file, points);
  writefile(outpath, file, points, nfont2x);
  puts("Baking hi-res font into bitmap... OK");

  return 0;
}
