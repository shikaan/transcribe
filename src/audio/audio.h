#pragma once

#include "miniaudio.h"
#include "types.h"
#include <stdbool.h>

typedef struct {
  float *buf;
  unsigned long long len;
  unsigned long long cap;
} samples_t;

typedef enum {
  TEMPO_100,
  TEMPO_75,
  TEMPO_50,
  TEMPO_25,

  TEMPOS
} tempo_t;

enum {
  MIN_TEMPO = TEMPO_25,
  MAX_TEMPO = TEMPO_100,
  MAX_CHANNELS = 4,
};

void *samples_alloc(unsigned int len, samples_t *s); 

result_t audio_open(const char *path);
void audio_play(void);
void audio_isplaying(bool *p);
void audio_pause(void);
void audio_stop(void);
void audio_samples(samples_t **s);
void audio_times(float *total, float *now);
void audio_seek(float seconds);
void audio_setloopregion(float a, float b);
void audio_loopregion(float *a, float *b);
void audio_setlooping(bool l);
void audio_islooping(bool *l);
void audio_settempo(tempo_t t);
void audio_tempo(tempo_t *t);
void audio_setpitch(float p);
void audio_pitch(float *p);
void audio_unint(void);
