#include "audio/audio.h"

#include "thread/thread.h"
#include "utils.h"
#include <assert.h>
#include <rubberband/rubberband-c.h>
#include <stdio.h>
#include <stdlib.h>

enum { CHUNK_SIZE = 65536 };
static const float TEMPO_FACTORS[TEMPOS] = {
    [TEMPO_100] = 1,
    [TEMPO_50] = 0.5F,
    [TEMPO_25] = 0.25F,
    [TEMPO_75] = 0.75F,
};

typedef struct {
  ma_uint32 rate;
  ma_uint32 nchannels;
  ma_uint64 nframes;
  const samples_t *rawsamples;
  tempo_t tempo;
} samplerargs_t;

static samples_t sampler_samples[TEMPOS] = {0};
static thread_t sampler_thread;
static int sampler_nstretched = 0;

// Fills `slice[0..ch]` with pointers into each channel of `s` at `offset`.
// Produces the float** layout rubberband expects for chunked processing.
static void sampler_torubberband(samples_t *s, float **slice, ma_uint32 ch,
                                 ma_uint64 offset) {
  for (ma_uint32 c = 0; c < ch; c++)
    slice[c] = s[c].buf + offset;
}

// Splits interleaved PCM into per-channel planar buffers:
// [L,R,L,R] -> [[L,L],[R,R]]
static void sampler_deinterleave(const float *in, samples_t *out, ma_uint64 len,
                                 ma_uint32 ch) {
  for (ma_uint32 c = 0; c < ch; c++) {
    for (ma_uint64 i = 0; i < len; i++)
      out[c].buf[i] = in[(i * ch) + c];
  }
}

// Merges per-channel planar buffers back into interleaved PCM: [[L,L],[R,R]] ->
// [L,R,L,R].
static void sampler_interleave(samples_t *in, float *out, ma_uint64 len,
                               ma_uint32 ch) {
  for (ma_uint64 i = 0; i < len; i++) {
    for (ma_uint32 c = 0; c < ch; c++)
      out[(i * ch) + c] = in[c].buf[i];
  }
}

// Time-stretches rawsamples to the given tempo using RubberBand (offline mode).
// Writes interleaved output into samples[t].
static result_t sampler_stretchone(samplerargs_t *args, samples_t *out) {
  ma_uint32 nchannels = args->nchannels;
  ma_uint32 rate = args->rate;
  ma_uint64 nframes = args->nframes;
  tempo_t t = args->tempo;
  const samples_t *in = args->rawsamples;

  assert(nchannels > 0 && "sampler_stretchone: nchannels must be non zero");
  assert(rate > 0 && "sampler_stretchone: rate must be non zero");
  assert(nframes > 0 && "sampler_stretchone: nframes must be non zero");

  double ratio = 1.0 / (double)TEMPO_FACTORS[t];
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

  if (!samples_alloc((unsigned int)(outnframes * nchannels), out)) {
    result = ERR_AUDIO_ALLOC;
    goto cleanup;
  }

  sampler_deinterleave(in->buf, planar, nframes, nchannels);

  for (ma_uint64 offset = 0; offset < nframes; offset += CHUNK_SIZE) {
    unsigned int chunk = min((unsigned int)(nframes - offset), CHUNK_SIZE);
    int last = (offset + chunk >= nframes);
    sampler_torubberband(planar, slice, nchannels, offset);
    rubberband_study(rb, (const float *const *)slice, chunk, last);
  }

  ma_uint64 retrieved = 0;
  for (ma_uint64 offset = 0; offset < nframes; offset += CHUNK_SIZE) {
    unsigned int chunk = min((unsigned int)(nframes - offset), CHUNK_SIZE);
    int last = (offset + chunk >= nframes);

    sampler_torubberband(planar, slice, nchannels, offset);
    rubberband_process(rb, (const float *const *)slice, chunk, last);

    int avail = rubberband_available(rb);
    while (avail > 0) {
      unsigned int remainder = (unsigned int)(outnframes - retrieved);
      chunk = min((unsigned int)avail, remainder);
      if (!chunk)
        break;
      sampler_torubberband(outplanar, slice, nchannels, retrieved);
      retrieved += rubberband_retrieve(rb, slice, chunk);
      avail = rubberband_available(rb);
    }
  }

  sampler_interleave(outplanar, out->buf, retrieved, nchannels);

cleanup:
  sampler_nstretched++;
  rubberband_delete(rb);
  for (int c = 0; c < MAX_CHANNELS; c++) {
    free(planar[c].buf);
    free(outplanar[c].buf);
  }
  return result;
}

static void sampler_worker(void *rawargs) {
  samplerargs_t *args = (samplerargs_t *)rawargs;
  sampler_stretchone(args, &sampler_samples[args->tempo]);
}

// Spawns one thread per non-100% tempo and stretches them in parallel.
static void sampler_stretchall(void *rawargs) {
  samplerargs_t sampler_args = *(samplerargs_t *)rawargs;
  const tempo_t tempos[] = {TEMPO_25, TEMPO_50, TEMPO_75};
  thread_t threads[TEMPOS - 1] = {0};

  // TEMPO_100 is natural tempo, does not need stretching
  sampler_samples[TEMPO_100] = *sampler_args.rawsamples;
  sampler_nstretched = 1;

  // Static allocation needed to keep the reference stable when threaded
  static samplerargs_t workerargs[len(threads)];

  for (size_t i = 0; i < len(threads); i++) {
    workerargs[i] = sampler_args;
    workerargs[i].tempo = tempos[i];
    thread_create(&threads[i], sampler_worker, &workerargs[i]);
  }

  for (size_t i = 0; i < len(threads); i++) {
    assert(threads[i] && "sampler_stretchall: thread must be initialized");
    thread_join(threads[i]);
  }
}

// Blocks until the background stretch thread completes, if one is running.
void sampler_await(void) {
  if (sampler_thread && sampler_nstretched < TEMPOS) {
    thread_join(sampler_thread);
  }
}

void sampler_isstretching(bool *s) { *s = sampler_nstretched < TEMPOS; }

void sampler_uninit(void) {
  for (int i = 0; i < TEMPOS; i++)
    sampler_samples[i].len = 0;
}

void sampler_init(ma_uint64 nframes, ma_uint32 nchannels, ma_uint32 rate,
                  const samples_t *raw_samples) {
  // Static allocation needed to keep the reference stable when threaded
  static samplerargs_t sampler_args;
  sampler_args.nchannels = nchannels;
  sampler_args.nframes = nframes;
  sampler_args.rate = rate;
  sampler_args.rawsamples = raw_samples;

  thread_create(&sampler_thread, sampler_stretchall, &sampler_args);
}

void sampler_samplesat(tempo_t t, samples_t* s) {
  *s = sampler_samples[t];
}
