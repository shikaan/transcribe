#include "audio/audio.h"

#include "audio/shifter.c"
#include "audio/sampler.c"

#include "types.h"
#include <assert.h>
#include <math.h>
#include <miniaudio.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FRAMES_PER_SAMPLE = 128 };

static ma_decoder decoder = {0};
static ma_device device = {0};
static ma_uint32 rate = 0;
static ma_uint32 nchannels = 2;
static ma_uint64 nframes = 0;

static samples_t rawsamples = {0};
static samples_t waveform = {0};
static tempo_t tempo = TEMPO_100;
static float pitch = 0.0F; // In semitones. 0.5f is a 1/4 tone.

static ma_uint64 cursor = 0;
static bool playing = false;
static bool looping = false;
static ma_uint64 loop_start = 0;
static ma_uint64 loop_end = 0;

// Downsamples rawframes into a buffer to be used to render the waveform
static result_t waveform_init(void) {
  unsigned int nsamples = (unsigned int)nframes / FRAMES_PER_SAMPLE;
  if (!samples_alloc(nsamples, &waveform))
    return ERR_AUDIO_ALLOC;

  for (ma_uint64 i = 0; i < waveform.len; i++) {
    float peak = 0.0F;
    for (ma_uint64 j = 0; j < FRAMES_PER_SAMPLE; j++) {
      float v = 0.0F;
      for (ma_uint32 c = 0; c < nchannels; c++) {
        v += fabsf(
            rawsamples.buf[(((i * FRAMES_PER_SAMPLE) + j) * nchannels) + c]);
      }
      v /= (float)nchannels;
      if (v > peak)
        peak = v;
    }
    waveform.buf[i] = peak;
  }

  return OK;
}

// Reads samples[tempo] through the shifter and writes to output
void callback(ma_device *_dev, void *out, const void *_in,
              ma_uint32 nchunkframes) {
  (void)_dev;
  (void)_in;
  float *output = (float *)out;
  samples_t samples;
  sampler_samplesat(tempo, &samples);

  if (!playing || samples.buf == NULL) {
    memset(output, 0, (size_t)(nchunkframes * nchannels) * sizeof(float));
    return;
  }

  ma_uint64 total = samples.len / nchannels;
  for (ma_uint32 i = 0; i < nchunkframes; i++) {
    if (looping && loop_end > loop_start && cursor >= loop_end)
      cursor = loop_start;

    if (cursor >= total) {
      playing = false;
      cursor = 0;

      // Shortcircuit and zeroout the input stream when we're at the end
      for (unsigned int c = 0; c < nchannels; c++)
        shifter_inzero(c, i, nchunkframes);

      return;
    }

    for (ma_uint32 c = 0; c < nchannels; c++)
      shifter_inset(c, i, samples.buf[(cursor * nchannels) + c]);

    cursor++;
  }
  shifter_shift(nchannels, nchunkframes, output);
}

result_t audio_open(const char *path) {
  assert(path && "audio_open: path must be non null");
  audio_unint();

  ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 0, 0);
  if (ma_decoder_init_file(path, &dcfg, &decoder) != MA_SUCCESS)
    return ERR_AUDIO_OPENFILE;

  nchannels = decoder.outputChannels;
  rate = decoder.outputSampleRate;

  if (ma_decoder_get_length_in_pcm_frames(&decoder, &nframes) != MA_SUCCESS)
    return ERR_AUDIO_DECODE;
 
  unsigned int nrawsamples = (unsigned int)nframes * nchannels;
  if (!samples_alloc(nrawsamples, &rawsamples))
    return ERR_AUDIO;

  ma_uint64 frames_read = 0;
  ma_decoder_read_pcm_frames(&decoder, rawsamples.buf, nframes, &frames_read);
  rawsamples.len = frames_read * nchannels;
  ma_decoder_seek_to_pcm_frame(&decoder, 0);
 
  result_t result = waveform_init();
  if (result != OK) return result;
   
  pitch = 0;
  ma_uint32 nperiod = 0;
  shifter_init(nchannels, rate, &nperiod);
  sampler_init(nframes, nchannels, rate, &rawsamples);

  ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
  cfg.playback.format = ma_format_f32;
  cfg.playback.channels = nchannels;
  cfg.sampleRate = rate;
  cfg.periodSizeInFrames = nperiod;
  cfg.dataCallback = callback;
  cfg.pUserData = NULL;

  if (ma_device_init(NULL, &cfg, &device) != MA_SUCCESS)
    return ERR_AUDIO_INIT;
  if (ma_device_start(&device) != MA_SUCCESS)
    return ERR_AUDIO_INIT;

  return OK;
}

void audio_play(void) { playing = true; }

void audio_pause(void) { playing = false; }

void audio_stop(void) {
  playing = false;
  cursor = 0;
}

void audio_isplaying(bool *p) {
  assert(p && "audio_isplaying: return value must be non-null");
  *p = playing;
}

void audio_unint(void) {
  sampler_await();

  playing = false;
  cursor = 0;
  looping = false;
  loop_start = 0;
  loop_end = 0;
  pitch = 0;
  waveform.len = 0;
  rawsamples.len = 0;

  ma_device_uninit(&device);
  ma_decoder_uninit(&decoder);
  shifter_uninit();
  sampler_uninit();
}

void audio_samples(samples_t **s) {
  assert(s && "audio_samples: return value must be non-null");
  *s = &waveform;
}

void audio_times(float *total, float *now) {
  assert(now || total && "audio_time: one of total and now must be not null");
  if (now)
    *now = (float)cursor / (float)rate * TEMPO_FACTORS[tempo];
  if (total)
    *total = rate > 0 ? (float)nframes / (float)rate : 0;
}

void audio_seek(float seconds) {
  cursor = (ma_uint64)(seconds / TEMPO_FACTORS[tempo] * (float)rate);
}

void audio_setloopregion(float a, float b) {
  assert(a < b && "audio_setloopregion: begnning must be smaller than end");
  loop_start = (ma_uint64)(a / TEMPO_FACTORS[tempo] * (float)rate);
  loop_end = (ma_uint64)(b / TEMPO_FACTORS[tempo] * (float)rate);
}

void audio_loopregion(float *a, float *b) {
  if (a)
    *a = (float)loop_start / (float)rate * TEMPO_FACTORS[tempo];
  if (b)
    *b = (float)loop_end / (float)rate * TEMPO_FACTORS[tempo];
}

void audio_setlooping(bool loop) { looping = loop; }

void audio_islooping(bool *loop) { *loop = looping; }

void audio_settempo(tempo_t t) {
  sampler_await();

  float la = 0;
  float lb = 0;
  bool was_looping = false, was_playing = false;
  float now = 0;
  audio_loopregion(&la, &lb);
  audio_islooping(&was_looping);
  audio_isplaying(&was_playing);
  audio_times(NULL, &now);

  tempo = t;

  if (was_looping && la < lb) {
    audio_setloopregion(la, lb);
    audio_setlooping(true);
  }
  audio_seek(now);
  if (was_playing)
    audio_play();
}

void audio_tempo(tempo_t *t) { *t = tempo; }

void audio_setpitch(float p) {
  pitch = p;
  shifter_setpitch(p);
}

void audio_pitch(float *p) {
  assert(p && "audio_pitch: return value must be non-null");
  *p = pitch;
}
