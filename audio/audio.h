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

/* Task-context callbacks run between DMA blocks. The capture sink returns false
 * to end recording normally. The source fills every requested stereo frame. */
typedef bool (*audio_capture_sink_t)(void *context, const int16_t *pcm, size_t frames);
typedef void (*audio_play_source_t)(void *context, int16_t *pcm, size_t frames);
bool audio_record_blocks(audio_capture_sink_t sink, void *context, size_t max_frames);
bool audio_play_blocks(audio_play_source_t source, void *context, size_t frames);

#endif
