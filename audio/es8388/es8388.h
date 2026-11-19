#ifndef ES8388_H
#define ES8388_H

#include <stdbool.h>
#include <stddef.h>
#include "stm32f7xx_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        ES8388_INPUT_MIC,
        ES8388_INPUT_LINE_IN,
    } es8388_input_t;

    typedef enum
    {
        ES8388_OUTPUT_NONE,
        ES8388_OUTPUT_SPEAKER,
        ES8388_OUTPUT_HEADPHONE,
        ES8388_OUTPUT_BOTH,
    } es8388_output_t;

    /* Call after hardware_init(). Sets up I2C2 and SAI1, and leaves the codec DAC muted. */
    HAL_StatusTypeDef es8388_init(void);
    HAL_StatusTypeDef es8388_set_input(es8388_input_t input);
    /* Mute playback before changing the powered output paths. */
    HAL_StatusTypeDef es8388_set_output(es8388_output_t output);
    /* 0 to 100 maps to -96 to 0 dB. Use es8388_set_mute() for silence. */
    HAL_StatusTypeDef es8388_set_volume(uint8_t volume);
    HAL_StatusTypeDef es8388_set_mute(bool mute);
    HAL_StatusTypeDef es8388_record(int16_t *pcm, size_t frames);
    HAL_StatusTypeDef es8388_play(const int16_t *pcm, size_t frames);

#ifdef __cplusplus
}
#endif

#endif
