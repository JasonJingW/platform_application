#include "es8388.h"
#include "bsp_iic.h"
#include "bsp_sai.h"

#define ES8388_IIC_ADDRESS (0x10U) /* AD0 is tied low on Apollo V2+. */
#define ES8388_DAC_MUTE (0x04U)
#define ES8388_DAC_LINK_VOLUME (0x08U)
#define ES8388_DAC_SOFT_RAMP (0x20U)
#define ES8388_DAC_ATTENUATION_MAX (192U) /* 0.5 dB steps. */
#define ES8388_DEFAULT_VOLUME (70U)

enum
{
    REG_CONTROL1 = 0x00,
    REG_CONTROL2 = 0x01,
    REG_CHIP_POWER = 0x02,
    REG_ADC_POWER = 0x03,
    REG_DAC_POWER = 0x04,
    REG_SERIAL_MODE = 0x08,
    REG_ADC_PGA = 0x09,
    REG_ADC_INPUT = 0x0A,
    REG_ADC_MUX = 0x0B,
    REG_ADC_FORMAT = 0x0C,
    REG_ADC_RATIO = 0x0D,
    REG_ADC_RAMP = 0x0F,
    REG_ADC_VOLUME_L = 0x10,
    REG_ADC_VOLUME_R = 0x11,
    REG_ADC_ALC = 0x12,
    REG_DAC_FORMAT = 0x17,
    REG_DAC_RATIO = 0x18,
    REG_DAC_RAMP = 0x19,
    REG_DAC_VOLUME_L = 0x1A,
    REG_MIXER_INPUT = 0x26,
    REG_MIXER_L_TO_L = 0x27,
    REG_MIXER_R_TO_L = 0x28,
    REG_MIXER_L_TO_R = 0x29,
    REG_MIXER_R_TO_R = 0x2A,
    REG_LRCK_CONTROL = 0x2B,
    REG_LOUT1_VOLUME = 0x2E,
    REG_ROUT1_VOLUME = 0x2F,
    REG_LOUT2_VOLUME = 0x30,
};

typedef struct
{
    uint8_t reg;
    uint8_t value;
} es8388_reg_value_t;

static SAI_HandleTypeDef es8388_tx;
static SAI_HandleTypeDef es8388_rx;

static HAL_StatusTypeDef es8388_write(uint8_t reg, uint8_t value)
{
    return bsp_iic_write_reg(ES8388_IIC_ADDRESS, reg, value);
}

static HAL_StatusTypeDef es8388_write_sequence(const es8388_reg_value_t *values, size_t count)
{
    for (size_t index = 0; index < count; ++index)
    {
        HAL_StatusTypeDef status = es8388_write(values[index].reg, values[index].value);
        if (status != HAL_OK)
        {
            return status;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef es8388_set_input(es8388_input_t input)
{
    if (input != ES8388_INPUT_MIC && input != ES8388_INPUT_LINE_IN)
    {
        return HAL_ERROR;
    }
    const bool mic = input == ES8388_INPUT_MIC;
    const es8388_reg_value_t values[] = {
        {REG_ADC_PGA, mic ? 0x44U : 0x00U},   /* +12 dB for MIC, 0 dB for LINE_IN. */
        {REG_ADC_INPUT, mic ? 0xF0U : 0x50U}, /* Differential input 1 or stereo input 2. */
        {REG_ADC_MUX, 0x02U},                 /* Use LINPUT1-RINPUT1 in differential mode. */
        {REG_ADC_FORMAT,
         mic ? 0x4CU : 0x0CU}, /* 16-bit I2S; duplicate the mono MIC on both slots. */
    };
    return es8388_write_sequence(values, sizeof(values) / sizeof(values[0]));
}

HAL_StatusTypeDef es8388_set_output(es8388_output_t output)
{
    uint8_t power;
    switch (output)
    {
    case ES8388_OUTPUT_NONE:
        power = 0xC0U;
        break;
    case ES8388_OUTPUT_SPEAKER:
        power = 0x08U; /* LOUT2 feeds MD8002A. */
        break;
    case ES8388_OUTPUT_HEADPHONE:
        power = 0x30U; /* LOUT1 and ROUT1 feed PHONE. */
        break;
    case ES8388_OUTPUT_BOTH:
        power = 0x38U;
        break;
    default:
        return HAL_ERROR;
    }
    return es8388_write(REG_DAC_POWER, power);
}

HAL_StatusTypeDef es8388_set_volume(uint8_t volume)
{
    if (volume > 100U)
    {
        return HAL_ERROR;
    }
    const uint8_t attenuation =
        (uint8_t)(((100U - volume) * ES8388_DAC_ATTENUATION_MAX + 50U) / 100U);
    return es8388_write(REG_DAC_VOLUME_L, attenuation);
}

HAL_StatusTypeDef es8388_set_mute(bool mute)
{
    uint8_t value;
    HAL_StatusTypeDef status = bsp_iic_read_reg(ES8388_IIC_ADDRESS, REG_DAC_RAMP, &value);
    if (status != HAL_OK)
    {
        return status;
    }
    value = mute ? (uint8_t)(value | ES8388_DAC_MUTE) : (uint8_t)(value & ~ES8388_DAC_MUTE);
    return es8388_write(REG_DAC_RAMP, value);
}

HAL_StatusTypeDef es8388_record(int16_t *pcm, size_t frames)
{
    const HAL_StatusTypeDef status = es8388_set_mute(true);
    if (status != HAL_OK)
    {
        return status;
    }
    return bsp_sai_record(&es8388_tx, &es8388_rx, pcm, frames);
}

HAL_StatusTypeDef es8388_play(const int16_t *pcm, size_t frames)
{
    HAL_StatusTypeDef status = es8388_set_mute(false);
    if (status == HAL_OK)
    {
        status = bsp_sai_play(&es8388_tx, pcm, frames);
    }
    const HAL_StatusTypeDef mute_status = es8388_set_mute(true);
    return status == HAL_OK ? mute_status : status;
}

HAL_StatusTypeDef es8388_init(void)
{
    static const es8388_reg_value_t startup[] = {
        {REG_DAC_RAMP, ES8388_DAC_SOFT_RAMP | ES8388_DAC_LINK_VOLUME | ES8388_DAC_MUTE | 0x02U},
        {REG_SERIAL_MODE, 0x00U},  /* SAI1 supplies audio clocks. */
        {REG_CHIP_POWER, 0xF3U},   /* Hold ADC/DAC state machines stopped during setup. */
        {REG_DAC_POWER, 0xC0U},    /* Disable DACs and output drivers while routing. */
        {REG_LRCK_CONTROL, 0x80U}, /* Shared ADC and DAC frame clock. */
        {REG_CONTROL1, 0x05U},
        {REG_CONTROL2, 0x40U},     /* Enable analog reference and bias. */
        {REG_ADC_POWER, 0x00U},    /* Power the ADC and analog input. */
        {REG_ADC_RATIO, 0x02U},    /* MCLK = 256 * LRCK. */
        {REG_ADC_RAMP, 0x20U},     /* ADC unmuted, digital gain ramp enabled. */
        {REG_ADC_VOLUME_L, 0x00U}, /* 0 dB digital gain. */
        {REG_ADC_VOLUME_R, 0x00U},
        {REG_ADC_ALC, 0x00U},    /* Fixed microphone gain. */
        {REG_DAC_FORMAT, 0x18U}, /* 16-bit I2S. */
        {REG_DAC_RATIO, 0x02U},  /* MCLK = 256 * LRCK. */
        {REG_MIXER_INPUT, 0x00U},
        {REG_MIXER_L_TO_L, 0xB8U}, /* Play each DAC on its own channel. */
        {REG_MIXER_R_TO_L, 0x38U},
        {REG_MIXER_L_TO_R, 0x38U},
        {REG_MIXER_R_TO_R, 0xB8U},
        {REG_LOUT1_VOLUME, 0x1EU}, /* 0 dB analog output gain. */
        {REG_ROUT1_VOLUME, 0x1EU},
        {REG_LOUT2_VOLUME, 0x1EU},
    };
    HAL_StatusTypeDef status = bsp_iic_init();
    if (status != HAL_OK)
    {
        return status;
    }
    status = bsp_sai_init(&es8388_tx, &es8388_rx);
    if (status != HAL_OK)
    {
        return status;
    }
    status = es8388_write_sequence(startup, sizeof(startup) / sizeof(startup[0]));
    if (status == HAL_OK)
    {
        status = es8388_set_input(ES8388_INPUT_MIC);
    }
    if (status == HAL_OK)
    {
        status = es8388_set_volume(ES8388_DEFAULT_VOLUME);
    }
    if (status == HAL_OK)
    {
        status = es8388_set_output(ES8388_OUTPUT_SPEAKER);
    }
    if (status == HAL_OK)
    {
        status = es8388_write(REG_CHIP_POWER, 0x00U);
    }
    return status;
}
