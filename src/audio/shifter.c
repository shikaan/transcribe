#include "audio.h"

#include "rubberband/rubberband-c.h"
#include <math.h>
#include <string.h>

enum { 
  SHIFTER_BLOCK_SIZE = 4096
};

static RubberBandLiveState rblive = 0;
// Pre-allocated scratch buffers for the audio callback 
static float shifter_in[MAX_CHANNELS][SHIFTER_BLOCK_SIZE] = {0};
static float shifter_out[MAX_CHANNELS][SHIFTER_BLOCK_SIZE] = {0};
static const float *rb_in_ptrs[MAX_CHANNELS] = {0};
static float *rb_out_ptrs[MAX_CHANNELS] = {0};

void shifter_setpitch(float pitch) {
  rubberband_live_set_pitch_scale(rblive, pow(2.0, (double)pitch / 12.0));
}

void shifter_init(ma_uint32 nchannels, ma_uint32 rate, ma_uint32 *nperiod) {
  int rbflags = RubberBandOptionEngineFiner | RubberBandOptionThreadingNever;
  rblive = rubberband_live_new(rate, nchannels, rbflags);
  *nperiod = rubberband_live_get_block_size(rblive);
  for (unsigned int c = 0; c < nchannels; c++) {
    rb_in_ptrs[c] = shifter_in[c];
    rb_out_ptrs[c] = shifter_out[c];
  }
}

// Set a frame value in a given channel in the input stream
void shifter_inset(unsigned int channel, unsigned int frame, float value) {
  shifter_in[channel][frame] = value;
}

// Zero out a chunk of the input stream for a given channel
void shifter_inzero(unsigned int channel, unsigned int start,
                    unsigned int end) {
  memset(&shifter_in[channel][start], 0, (end - start) * sizeof(float));
}

// Perform shift and write values in the out stream
void shifter_shift(size_t nchannels, size_t nframes, float* out) {
  rubberband_live_shift(rblive, rb_in_ptrs, rb_out_ptrs);
  
  for (unsigned int i = 0; i < nframes; i++) {
    for (unsigned int c = 0; c < nchannels; c++)
      out[(i * nchannels) + c] = shifter_out[c][i];
  }
}

void shifter_uninit(void) {
  if (rblive) {
    rubberband_live_delete(rblive);
    rblive = NULL;
  }
}
