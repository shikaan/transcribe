#include "audio.h"
#include "miniaudio.h"
#include "types.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ma_result result = {0};
static ma_engine engine = {0};
static ma_sound sound = {0};

const ma_uint64 framespersample = 128;
static samples_t samples = {0, 0};

static result_t audio_sample(void) {
  // TODO: how to assert sound and engine are initialized here?
  result_t res = OK;

  ma_format fmt;
  ma_uint32 channels, rate;
  ma_sound_get_data_format(&sound, &fmt, &channels, &rate, NULL, 0);

  ma_uint64 nframes;
  ma_sound_get_length_in_pcm_frames(&sound, &nframes);

  float *buf = malloc(nframes * channels * sizeof(float));
  if (!buf) {
    res = ERR_AUDIO;
    goto done;
  }

  ma_uint64 n;
  ma_data_source *src = ma_sound_get_data_source(&sound);
  ma_data_source_read_pcm_frames(src, buf, nframes, &n);
  ma_sound_seek_to_pcm_frame(&sound, 0); // rewind after reading
  if (n != nframes) {
    res = ERR_AUDIO_DECODE;
    goto done;
  }

  size_t sampleslen = nframes / framespersample;
  if (samples.buf) {
    if (samples.len >= sampleslen) {
      memset(samples.buf, 0, samples.len * sizeof(float));
      samples.len = sampleslen;
    } else {
      samples.len = sampleslen;
      samples.buf = realloc(samples.buf, samples.len * sizeof(float));
      if (!samples.buf) {
        res = ERR_AUDIO;
        goto done;
      }
    }
  } else {
    samples.len = sampleslen;
    samples.buf = malloc(samples.len * sizeof(float));
    if (!samples.buf) {
      res = ERR_AUDIO;
      goto done;
    }
  }

  for (unsigned int i = 0; i < samples.len; i++) {
    float peak = 0.0f;
    for (ma_uint64 j = 0; j < framespersample; j++) {
      // TODO: we could have this per channel instead of collapsing into mono
      float v = 0.0f;
      for (ma_uint32 c = 0; c < channels; c++)
        v += fabsf(buf[(i * framespersample + j) * channels + c]);
      v /= (float)channels;
      if (v > peak)
        peak = v;
    }
    samples.buf[i] = peak;
  }

done:
  free(buf);
  return res;
}

result_t audio_open(const char *path) {
  assert(path && "audio_open: path must be non null");
  audio_deinit();

  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    return ERR_AUDIO_INIT;
  }

  result = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE, NULL,
                                   NULL, &sound);
  if (result != MA_SUCCESS) {
    return ERR_AUDIO_OPENFILE;
  }

  return audio_sample();
}

void audio_play(void) { ma_sound_start(&sound); }

void audio_pause(void) { ma_sound_stop(&sound); }

void audio_stop(void) {
  ma_sound_stop(&sound);
  ma_sound_seek_to_pcm_frame(&sound, 0);
}

void audio_isplaying(bool *b) {
  assert(b && "audio_isplaying: return value must be non-null");
  *b = ma_sound_is_playing(&sound);
}

void audio_deinit(void) {
  free(samples.buf);
  samples.buf = NULL;

  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
}

void audio_samples(samples_t **s) {
  assert(s && "audio_samples: return value must be non-null");
  *s = &samples;
}

void audio_times(float *total, float *now) {
  assert(now || total && "audio_time: one of total and now must be not null");
  if (now) ma_sound_get_cursor_in_seconds(&sound, now);
  if (total) ma_sound_get_length_in_seconds(&sound, total);
}

void audio_seek(float time) { ma_sound_seek_to_second(&sound, time); }
