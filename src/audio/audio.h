#pragma once

#include "miniaudio.h"
#include "types.h"
#include <stdbool.h>

result_t audio_open(const char *path);
void audio_play(void);
void audio_deinit(void);
void audio_pause(void);
void audio_stop(void);
bool audio_isplaying(void);
