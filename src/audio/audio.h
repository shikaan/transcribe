#pragma once

#include "miniaudio.h"
#include "types.h"
#include <stdbool.h>

typedef struct {
  float *buf;
  unsigned long len;
} samples_t;

typedef struct { float a, b; } loopregion_t;

void audio_play(void);
void audio_deinit(void);
void audio_pause(void);
void audio_stop(void);

result_t audio_open(const char *path);

void audio_isplaying(bool *isplaying);
void audio_samples(samples_t **samples);
void audio_times(float *total, float *current);
void audio_seek(float time);

void audio_setloopregion(float a, float b);
void audio_setloop(bool loop);
void audio_islooping(bool *loop);
