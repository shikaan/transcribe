#include "audio/audio.h"

#include "audio/shifter.c"

#include "thread/thread.h"
#include "types.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <miniaudio.h>
#include <rubberband/rubberband-c.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  CHUNK_SIZE = 65536,
  FRAMES_PER_SAMPLE = 128,
};

static float tempos[TEMPOS] = {
    [TEMPO_100] = 1,
    [TEMPO_50] = 0.5F,
    [TEMPO_25] = 0.25F,
    [TEMPO_75] = 0.75F,
};

static ma_decoder decoder = {0};
static ma_device device = {0};
static ma_uint32 rate = 0;
static ma_uint32 nchannels = 2;
static ma_uint64 nframes = 0;

static samples_t rawsamples = {0, 0, 0};
static samples_t waveform = {0, 0, 0};
static samples_t samples[TEMPOS] = {0};
static tempo_t tempo = TEMPO_100;
static float pitch = 0.0F; // In semitones. 0.5f is a 1/4 tone.

static ma_uint64 cursor = 0;
static bool playing = false;
static bool looping = false;
static ma_uint64 loop_start = 0;
static ma_uint64 loop_end = 0;

static thread_t stretchthread;
static bool stretching = false;

// Allocates or reuses a samples buffer of `len` floats. Reuses the existing
// allocation if capacity is sufficient; zeroes the content on reuse.
static void *samples_alloc(unsigned int len, samples_t *s) {
  assert(len > 0 && "samples_alloc: len must be > 0");
  if (s->buf) {
    if (s->cap >= len) {
      memset(s->buf, 0, s->len * sizeof(float));
      s->len = len;
    } else {
      float *buf = realloc(s->buf, len * sizeof(float));
      if (!buf) {
        // TODO: ERROR HANDLING
        return s->buf;
      }
      s->buf = buf;
      s->len = len;
      s->cap = len;
    }
  } else {
    s->buf = malloc(len * sizeof(float));
    s->len = len;
    s->cap = len;
  }
  return s->buf;
}

// Fills `slice[0..ch]` with pointers into each channel of `s` at `offset`.
// Produces the float** layout rubberband expects for chunked processing.
static void torubberband(samples_t *s, float **slice, ma_uint32 ch,
                         ma_uint64 offset) {
  for (ma_uint32 c = 0; c < ch; c++)
    slice[c] = s[c].buf + offset;
}

// Splits interleaved PCM into per-channel planar buffers:
// [L,R,L,R] -> [[L,L],[R,R]]
static void deinterleave(const float *in, samples_t *out, ma_uint64 len,
                         ma_uint32 ch) {
  for (ma_uint32 c = 0; c < ch; c++) {
    for (ma_uint64 i = 0; i < len; i++)
      out[c].buf[i] = in[(i * ch) + c];
  }
}

// Merges per-channel planar buffers back into interleaved PCM: [[L,L],[R,R]] ->
// [L,R,L,R].
static void interleave(samples_t *in, float *out, ma_uint64 len, ma_uint32 ch) {
  for (ma_uint64 i = 0; i < len; i++) {
    for (ma_uint32 c = 0; c < ch; c++)
      out[(i * ch) + c] = in[c].buf[i];
  }
}

// Time-stretches rawsamples to the given tempo using RubberBand (offline mode).
// Writes interleaved output into samples[t].
static result_t stretch(tempo_t t) {
  assert(nchannels > 0 && "stretch: nchannels must be non zero");
  double ratio = 1.0 / (double)tempos[t];
  samples_t planar[MAX_CHANNELS] = {0};
  samples_t outplanar[MAX_CHANNELS] = {0};
  float *slice[nchannels];
  result_t result = OK;

  int flags = RubberBandOptionProcessOffline | RubberBandOptionEngineFaster |
              RubberBandOptionThreadingNever;
  RubberBandState rb = rubberband_new(rate, nchannels, flags, ratio, 1.0);

  rubberband_set_expected_input_duration(rb, (unsigned int)nframes);
  rubberband_set_max_process_size(rb, CHUNK_SIZE);

  ma_uint64 outnframes =
      (ma_uint64)((double)nframes * ratio) + rubberband_get_latency(rb);

  for (ma_uint32 c = 0; c < nchannels; c++) {
    if (!samples_alloc((unsigned int)nframes, &planar[c])) {
      result = ERR_AUDIO_ALLOC;
      goto cleanup;
    }
    if (!samples_alloc((unsigned int)outnframes, &outplanar[c])) {
      result = ERR_AUDIO_ALLOC;
      goto cleanup;
    }
  }

  if (!samples_alloc((unsigned int)(outnframes * nchannels), &samples[t])) {
    result = ERR_AUDIO_ALLOC;
    goto cleanup;
  }

  deinterleave(rawsamples.buf, planar, nframes, nchannels);

  for (ma_uint64 offset = 0; offset < nframes; offset += CHUNK_SIZE) {
    unsigned int chunk = min((unsigned int)(nframes - offset), CHUNK_SIZE);
    int last = (offset + chunk >= nframes);
    torubberband(planar, slice, nchannels, offset);
    rubberband_study(rb, (const float *const *)slice, chunk, last);
  }

  ma_uint64 retrieved = 0;
  for (ma_uint64 offset = 0; offset < nframes; offset += CHUNK_SIZE) {
    unsigned int chunk = min((unsigned int)(nframes - offset), CHUNK_SIZE);
    int last = (offset + chunk >= nframes);

    torubberband(planar, slice, nchannels, offset);
    rubberband_process(rb, (const float *const *)slice, chunk, last);

    int avail = rubberband_available(rb);
    while (avail > 0) {
      unsigned int remainder = (unsigned int)(outnframes - retrieved);
      chunk = min((unsigned int)avail, remainder);
      if (!chunk)
        break;
      torubberband(outplanar, slice, nchannels, retrieved);
      retrieved += rubberband_retrieve(rb, slice, chunk);
      avail = rubberband_available(rb);
    }
  }

  interleave(outplanar, samples[t].buf, retrieved, nchannels);

cleanup:
  rubberband_delete(rb);
  for (int c = 0; c < MAX_CHANNELS; c++) {
    free(planar[c].buf);
    free(outplanar[c].buf);
  }
  return result;
}

static void worker(void *arg) { stretch(*(tempo_t *)arg); }

// Spawns one thread per non-100% tempo and stretches them in parallel.
static void stretchall(void *none) {
  (void)none;
  tempo_t ts[] = {TEMPO_25, TEMPO_50, TEMPO_75};
  thread_t threads[3] = {0};
  for (int i = 0; i < 3; i++)
    thread_create(&threads[i], worker, &ts[i]);
  for (int i = 0; i < 3; i++) {
    assert(threads[i] && "stretchall: thread must be initialized");
    thread_join(threads[i]);
  }
}

// Blocks until the background stretch thread completes, if one is running.
static void awaitstretch(void) {
  if (stretching) {
    thread_join(stretchthread);
    stretching = false;
  }
}

// Reads all PCM frames from the decoder into rawsamples, then builds
// the waveform overview by taking the peak amplitude per FRAMES_PER_SAMPLE
// block.
static result_t rawsamples_init(void) {
  unsigned int nrawsamples = (unsigned int)nframes * nchannels;
  if (!samples_alloc(nrawsamples, &rawsamples))
    return ERR_AUDIO;

  ma_uint64 n;
  ma_decoder_read_pcm_frames(&decoder, rawsamples.buf, nframes, &n);
  ma_decoder_seek_to_pcm_frame(&decoder, 0);
  if (n != nframes)
    return ERR_AUDIO_DECODE;

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

  if (!playing || samples[tempo].buf == NULL) {
    memset(output, 0, (size_t)(nchunkframes * nchannels) * sizeof(float));
    return;
  }

  ma_uint64 total = samples[tempo].len / nchannels;
  for (ma_uint32 i = 0; i < nchunkframes; i++) {
    if (looping && loop_end > loop_start && cursor >= loop_end)
      cursor = loop_start;

    if (cursor >= total) {
      playing = false;
      cursor = 0;

      // Shortcircuit and zeroout the input strem when we're at the end
      for (unsigned int c = 0; c < nchannels; c++)
        shifter_inzero(c, i, nchunkframes);

      return;
    }

    for (ma_uint32 c = 0; c < nchannels; c++)
      shifter_inset(c, i, samples[tempo].buf[(cursor * nchannels) + c]);

    cursor++;
  }
  shifter_shift(nchannels, nchunkframes, output);
}

result_t audio_open(const char *path) {
  assert(path && "audio_open: path must be non null");
  audio_deinit();

  ma_decoder_config dcfg = ma_decoder_config_init(ma_format_f32, 0, 0);
  if (ma_decoder_init_file(path, &dcfg, &decoder) != MA_SUCCESS)
    return ERR_AUDIO_OPENFILE;

  nchannels = decoder.outputChannels;
  rate = decoder.outputSampleRate;

  if (ma_decoder_get_length_in_pcm_frames(&decoder, &nframes) != MA_SUCCESS)
    return ERR_AUDIO_DECODE;

  result_t result = rawsamples_init();
  if (result != OK) return result;

  // TEMPO_100 is raw, no stretching needed. 
  samples[TEMPO_100] = rawsamples;
 
  pitch = 0;
  ma_uint32 nperiod = 0;
  shifter_init(nchannels, rate, &nperiod);

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

  thread_create(&stretchthread, stretchall, NULL);
  stretching = true;

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

void audio_deinit(void) {
  awaitstretch();

  playing = false;
  cursor = 0;
  looping = false;
  loop_start = 0;
  loop_end = 0;
  pitch = 0;

  ma_device_uninit(&device);
  ma_decoder_uninit(&decoder);
  shifter_uninit();

  waveform.len = 0;
  rawsamples.len = 0;
  for (int i = 0; i < TEMPOS; i++)
    samples[i].len = 0;
}

void audio_samples(samples_t **s) {
  assert(s && "audio_samples: return value must be non-null");
  *s = &waveform;
}

void audio_times(float *total, float *now) {
  assert(now || total && "audio_time: one of total and now must be not null");
  if (now)
    *now = (float)cursor / (float)rate * tempos[tempo];
  if (total)
    *total = rate > 0 ? (float)nframes / (float)rate : 0;
}

void audio_seek(float seconds) {
  cursor = (ma_uint64)(seconds / tempos[tempo] * (float)rate);
}

void audio_setloopregion(float a, float b) {
  assert(a < b && "audio_setloopregion: begnning must be smaller than end");
  loop_start = (ma_uint64)(a / tempos[tempo] * (float)rate);
  loop_end = (ma_uint64)(b / tempos[tempo] * (float)rate);
}

void audio_loopregion(float *a, float *b) {
  if (a)
    *a = (float)loop_start / (float)rate * tempos[tempo];
  if (b)
    *b = (float)loop_end / (float)rate * tempos[tempo];
}

void audio_setlooping(bool loop) { looping = loop; }

void audio_islooping(bool *loop) { *loop = looping; }

void audio_settempo(tempo_t t) {
  awaitstretch();

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
