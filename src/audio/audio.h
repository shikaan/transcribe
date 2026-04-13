#pragma once

#include "miniaudio.h"
#include "types.h"
#include <stdbool.h>

typedef struct {
  float *buf;
  unsigned long len;
  unsigned long cap;
} samples_t;

typedef enum {
  TEMPO_100,
  TEMPO_75,
  TEMPO_50,
  TEMPO_25,

  TEMPOS
} tempo_t;

result_t audio_open(const char *path);
void audio_play(void);
void audio_isplaying(bool *isplaying);
void audio_pause(void);
void audio_stop(void);
void audio_samples(samples_t **samples);
void audio_times(float *total, float *current);
void audio_seek(float time);
void audio_setloopregion(float a, float b);
void audio_loopregion(float *a, float *b);
void audio_setlooping(bool loop);
void audio_islooping(bool *loop);
void audio_settempo(tempo_t t);
void audio_tempo(tempo_t *t);
void audio_deinit(void);
