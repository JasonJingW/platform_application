#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool audio_init(void);

/* Blocking task-context APIs. PCM is signed 16-bit, interleaved left/right at ~48 kHz.
 * One frame contains two samples. Only one operation may run at a time.
 * Returns false on invalid input, audio busy/uninitialized, or transfer failure. */
bool audio_record(int16_t *pcm, size_t frames);
bool audio_play(const int16_t *pcm, size_t frames);

#endif
