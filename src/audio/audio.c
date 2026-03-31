#include "audio.h"
#include "miniaudio.h"
#include "types.h"

static ma_result result;
static ma_engine engine;
static ma_sound sound;

result_t audio_open(const char *path) {
  result = ma_engine_init(NULL, &engine);
  if (result != MA_SUCCESS) {
    return ERR_AUDIO_INIT;
  }

  result = ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    return ERR_AUDIO_OPENFILE;
  }

  return OK;
}

void audio_play(void) { ma_sound_start(&sound); }

void audio_pause(void) { ma_sound_stop(&sound); }

void audio_stop(void) {
  ma_sound_stop(&sound);
  ma_sound_seek_to_pcm_frame(&sound, 0);
}

bool audio_isplaying(void) { return ma_sound_is_playing(&sound); }

void audio_deinit(void) {
  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
}
