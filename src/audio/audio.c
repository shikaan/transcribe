#include "audio/audio.h"
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

enum { CHUNK_SIZE = 65536, MAX_CHANNELS = 4, FRAMES_PER_SAMPLE = 128 };

static float tempos[TEMPOS] = {
    [TEMPO_100] = 1,
    [TEMPO_50] = 0.5F,
    [TEMPO_25] = 0.25F,
    [TEMPO_75] = 0.75F,
};

static ma_result maresult = {0};
static ma_engine engine = {0};
static ma_sound sound = {0};
static ma_audio_buffer audio = {0};
static ma_uint32 rate = 0;
static ma_uint32 nchannels = 0;
static ma_uint64 nframes = 0;

static samples_t rawsamples = {0, 0, 0};
static samples_t waveform = {0, 0, 0};
static samples_t samples[TEMPOS] = {0};
static tempo_t tempo = TEMPO_100;

static thread_t stretchthread;
static bool stretching = false;

// Allocates or reuses a samples buffer of `len` floats. Reuses the existing
// allocation if capacity is sufficient; zeroes the content on reuse.
static void *samples_alloc(unsigned int len, samples_t *s) {
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
      if (!chunk) break;
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
  // TEMPO_100 does not need stretching, it's the natural tempo.
  samples[TEMPO_100] = rawsamples;

  tempo_t ts[] = {TEMPO_25, TEMPO_50, TEMPO_75};
  thread_t threads[3] = {0};
  for (int i = 0; i < 3; i++)
    thread_create(&threads[i], worker, &ts[i]);
  for (int i = 0; i < 3; i++) {
    assert(threads[i]); 
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

// Reads all PCM frames from the decoded sound into rawsamples, then builds
// the waveform overview by taking the peak amplitude per FRAMES_PER_SAMPLE
// block.
static result_t sample(void) {
  unsigned int nrawsamples = (unsigned int)nframes * nchannels;
  if (!samples_alloc(nrawsamples, &rawsamples)) return ERR_AUDIO;

  ma_uint64 n;
  ma_data_source *src = ma_sound_get_data_source(&sound);
  ma_data_source_read_pcm_frames(src, rawsamples.buf, nframes, &n);
  ma_sound_seek_to_pcm_frame(&sound, 0); // rewind after reading
  if (n != nframes) return ERR_AUDIO_DECODE;

  unsigned int nsamples = (unsigned int)nframes / FRAMES_PER_SAMPLE;
  if (!samples_alloc(nsamples, &waveform)) return ERR_AUDIO;

  for (ma_uint64 i = 0; i < waveform.len; i++) {
    float peak = 0.0F;
    for (ma_uint64 j = 0; j < FRAMES_PER_SAMPLE; j++) {
      float v = 0.0F;
      for (ma_uint32 c = 0; c < nchannels; c++) {
        v += fabsf(
            rawsamples.buf[(((i * FRAMES_PER_SAMPLE) + j) * nchannels) + c]);
      }
      v /= (float)nchannels;
      if (v > peak) peak = v;
    }
    waveform.buf[i] = peak;
  }

  return OK;
}

result_t audio_open(const char *path) {
  assert(path && "audio_open: path must be non null");
  audio_deinit();

  maresult = ma_engine_init(NULL, &engine);
  if (maresult != MA_SUCCESS) return ERR_AUDIO_INIT;

  maresult = ma_sound_init_from_file(&engine, path, MA_SOUND_FLAG_DECODE, NULL,
                                   NULL, &sound);
  if (maresult != MA_SUCCESS) return ERR_AUDIO_OPENFILE;

  ma_data_source_set_looping(sound.pDataSource, false);
  ma_format fmt;
  ma_sound_get_data_format(&sound, &fmt, &nchannels, &rate, NULL, 0);
  ma_sound_get_length_in_pcm_frames(&sound, &nframes);

  result_t result = sample();
  if (result != OK) return result;

  thread_create(&stretchthread, stretchall, NULL);
  stretching = true;

  return OK;
}

void audio_play(void) { ma_sound_start(&sound); }

void audio_pause(void) { ma_sound_stop(&sound); }

void audio_stop(void) {
  ma_sound_stop(&sound);
  ma_sound_seek_to_pcm_frame(&sound, 0);
}

void audio_isplaying(bool *isplaying) {
  assert(isplaying && "audio_isplaying: return value must be non-null");
  *isplaying = ma_sound_is_playing(&sound);
}

void audio_deinit(void) {
  awaitstretch();

  waveform.len = 0;
  rawsamples.len = 0;
  for (int i = 0; i < TEMPOS; i++)
    samples[i].len = 0;

  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
  ma_audio_buffer_uninit(&audio);
}

void audio_samples(samples_t **s) {
  assert(s && "audio_samples: return value must be non-null");
  *s = &waveform;
}

void audio_times(float *total, float *now) {
  assert(now || total && "audio_time: one of total and now must be not null");
  if (now) {
    ma_uint64 cursor_frames;
    ma_sound_get_cursor_in_pcm_frames(&sound, &cursor_frames);
    *now = (float)cursor_frames / (float)rate * tempos[tempo];
  }
  if (total) {
    *total = rate > 0 ? (float)nframes / (float)rate : 0;
  }
}

void audio_seek(float time) {
  ma_uint64 frame = (ma_uint64)(time / tempos[tempo] * (float)rate);
  ma_sound_seek_to_pcm_frame(&sound, frame);
}

void audio_setloopregion(float a, float b) {
  assert(a < b && "audio_setloopregion: begnning must be smaller than end");
  ma_uint32 start = (ma_uint32)(a / tempos[tempo] * (float)rate);
  ma_uint32 end = (ma_uint32)(b / tempos[tempo] * (float)rate);
  ma_data_source_set_loop_point_in_pcm_frames(sound.pDataSource, start, end);
}

void audio_loopregion(float *a, float *b) {
  ma_uint64 start;
  ma_uint64 end;
  ma_data_source_get_loop_point_in_pcm_frames(sound.pDataSource, &start, &end);
  *a = (float)start / (float)rate * tempos[tempo];
  *b = (float)end / (float)rate * tempos[tempo];
}

void audio_setlooping(bool loop) {
  ma_data_source_set_looping(sound.pDataSource, loop);
}

void audio_islooping(bool *loop) {
  *loop = (bool)ma_data_source_is_looping(sound.pDataSource);
}

void audio_settempo(tempo_t t) {
  awaitstretch();
  // Read previous state to restore it after switching to stretched track
  float la = 0;
  float lb = 0;
  bool looping = false;
  bool playing = false;
  float now = 0;
  audio_loopregion(&la, &lb);
  audio_islooping(&looping);
  audio_isplaying(&playing);
  audio_times(NULL, &now);

  ma_sound_uninit(&sound);
  ma_audio_buffer_uninit(&audio);
  ma_audio_buffer_config cfg = ma_audio_buffer_config_init(
      ma_format_f32, nchannels, samples[t].len, samples[t].buf, NULL);
  ma_audio_buffer_init(&cfg, &audio);

  maresult = ma_sound_init_from_data_source(
      &engine, &audio, MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION,
      NULL, &sound);

  if (maresult == MA_SUCCESS) {
    tempo = t;
    if (looping && la < lb) {
      audio_setloopregion(la, lb);
      audio_setlooping(true);
    }
    audio_seek(now);
    if (playing) {
      audio_play();
    }
  }

  // FIXME: error handling!
}

void audio_tempo(tempo_t *t) { *t = tempo; }
