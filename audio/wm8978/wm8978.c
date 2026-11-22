#include "wm8978.h"
#include "project_config.h"
#include "bsp_iic.h"
#include "bsp_sai.h"

#define WM8978_ADDRESS (0x1AU)
#define WM8978_SPEAKER_GAIN (0x3FU) /* +6 dB */
#define WM8978_MIC_GAIN (0x38U)     /* +30 dB */
#define WM8978_VOLUME_UPDATE (0x100U)
#define WM8978_OUTPUT_MUTE (0x40U)
#define WM8978_VMID_SETTLE_MS (500U)

enum
{
    WM8978_RESET = 0,
    WM8978_POWER_1 = 1,
    WM8978_POWER_2 = 2,
    WM8978_POWER_3 = 3,
    WM8978_INTERFACE = 4,
    WM8978_CLOCK = 6,
    WM8978_DAC_CONTROL = 10,
    WM8978_ADC_CONTROL = 14,
    WM8978_BEEP_CONTROL = 43,
    WM8978_INPUT = 44,
    WM8978_LEFT_INPUT_GAIN = 45,
    WM8978_RIGHT_INPUT_GAIN = 46,
    WM8978_LEFT_ADC_BOOST = 47,
    WM8978_RIGHT_ADC_BOOST = 48,
    WM8978_OUTPUT = 49,
    WM8978_LEFT_MIXER = 50,
    WM8978_RIGHT_MIXER = 51,
    WM8978_LEFT_SPEAKER = 54,
    WM8978_RIGHT_SPEAKER = 55,
};

static SAI_HandleTypeDef sai_tx;
static SAI_HandleTypeDef sai_rx;

static HAL_StatusTypeDef wm8978_write(uint8_t reg, uint16_t value)
{
    const uint8_t first = (uint8_t)((reg << 1U) | (value >> 8U));
    const HAL_StatusTypeDef status = bsp_iic_write_reg(WM8978_ADDRESS, first, (uint8_t)value);
    if (status != HAL_OK)
    {
        RLOG_ERR("WM8978 register %u write failed (%u)", (unsigned)reg, (unsigned)status);
    }
    return status;
}

static HAL_StatusTypeDef wm8978_mute(bool mute)
{
    const uint16_t level = WM8978_SPEAKER_GAIN | (mute ? WM8978_OUTPUT_MUTE : 0U);
    const HAL_StatusTypeDef left = wm8978_write(WM8978_LEFT_SPEAKER, level);
    const HAL_StatusTypeDef right =
        wm8978_write(WM8978_RIGHT_SPEAKER, level | WM8978_VOLUME_UPDATE);
    return left == HAL_OK ? right : left;
}

HAL_StatusTypeDef wm8978_init(void)
{
    HAL_StatusTypeDef status = bsp_iic_init();
    if (status != HAL_OK)
    {
        RLOG_ERR("WM8978 I2C init failed (%u)", (unsigned)status);
        return status;
    }
    status = wm8978_write(WM8978_RESET, 0U);
    if (status != HAL_OK)
    {
        return status;
    }
    status = bsp_sai_init(&sai_tx, &sai_rx);
    if (status != HAL_OK)
    {
        RLOG_ERR("WM8978 SAI init failed (%u)", (unsigned)status);
        return status;
    }
    status = wm8978_mute(true);
    if (status != HAL_OK)
    {
        return status;
    }
    status = wm8978_write(WM8978_POWER_1, 0x003U); /* Charge VMID at low impedance. */
    if (status != HAL_OK)
    {
        return status;
    }
    vTaskDelay(pdMS_TO_TICKS(WM8978_VMID_SETTLE_MS));

    static const struct
    {
        uint8_t reg;
        uint16_t value;
    } config[] = {
        {WM8978_INTERFACE, 0x010U},   /* I2S, 16 bits. */
        {WM8978_CLOCK, 0x000U},       /* Slave, external MCLK. */
        {WM8978_DAC_CONTROL, 0x008U}, /* 128x oversampling. */
        {WM8978_ADC_CONTROL, 0x108U}, /* High-pass filter, 128x. */
        {WM8978_INPUT, 0x033U},       /* Differential mic inputs. */
        {WM8978_LEFT_INPUT_GAIN, WM8978_MIC_GAIN},
        {WM8978_RIGHT_INPUT_GAIN, WM8978_MIC_GAIN | WM8978_VOLUME_UPDATE},
        {WM8978_LEFT_ADC_BOOST, 0U},
        {WM8978_RIGHT_ADC_BOOST, 0U},
        {WM8978_BEEP_CONTROL, 0x010U}, /* Invert ROUT2 for bridged speaker. */
        {WM8978_OUTPUT, 0x002U},       /* Thermal shutdown, no boost. */
        {WM8978_LEFT_MIXER, 0x001U},   /* DAC only. */
        {WM8978_RIGHT_MIXER, 0x001U},
        {WM8978_POWER_3, 0x00FU}, /* DACs and mixers. */
        {WM8978_POWER_1, 0x019U}, /* Mic bias, analogue bias, VMID. */
        {WM8978_POWER_2, 0x03FU}, /* Input PGAs, boost stages, ADCs. */
        {WM8978_POWER_3, 0x06FU}, /* Also enable muted speaker amps. */
    };
    for (size_t i = 0U; i < sizeof(config) / sizeof(config[0]); ++i)
    {
        status = wm8978_write(config[i].reg, config[i].value);
        if (status != HAL_OK)
        {
            return status;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef wm8978_record(int16_t *pcm, size_t frames)
{
    const HAL_StatusTypeDef status = wm8978_mute(true);
    return status == HAL_OK ? bsp_sai_record(&sai_tx, &sai_rx, pcm, frames) : status;
}

HAL_StatusTypeDef wm8978_record_blocks(bool (*sink)(void *, const int16_t *, size_t), void *context,
                                       size_t frames)
{
    const HAL_StatusTypeDef status = wm8978_mute(true);
    return status == HAL_OK ? bsp_sai_record_blocks(&sai_tx, &sai_rx, sink, context, frames)
                            : status;
}

HAL_StatusTypeDef wm8978_play_blocks(void (*source)(void *, int16_t *, size_t), void *context,
                                     size_t frames)
{
    HAL_StatusTypeDef status = wm8978_mute(false);
    if (status == HAL_OK)
    {
        status = bsp_sai_play_blocks(&sai_tx, source, context, frames);
    }
    const HAL_StatusTypeDef mute_status = wm8978_mute(true);
    return status == HAL_OK ? mute_status : status;
}

static void wm8978_copy_playback(void *context, int16_t *pcm, size_t frames)
{
    const int16_t **source = context;
    const size_t samples = frames * 2U;
    memcpy(pcm, *source, samples * sizeof(*pcm));
    *source += samples;
}

HAL_StatusTypeDef wm8978_play(const int16_t *pcm, size_t frames)
{
    return wm8978_play_blocks(wm8978_copy_playback, &pcm, frames);
}
