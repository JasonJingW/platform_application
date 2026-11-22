#ifndef WM8978_H
#define WM8978_H

#include <stddef.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

HAL_StatusTypeDef wm8978_init(void);
HAL_StatusTypeDef wm8978_record(int16_t *pcm, size_t frames);
HAL_StatusTypeDef wm8978_play(const int16_t *pcm, size_t frames);
HAL_StatusTypeDef wm8978_record_blocks(bool (*sink)(void *, const int16_t *, size_t), void *context,
                                       size_t frames);
HAL_StatusTypeDef wm8978_play_blocks(void (*source)(void *, int16_t *, size_t), void *context,
                                     size_t frames);

#endif
