#include "audio.h"
#include "project_config.h"
#include "wm8978.h"
#include "FreeRTOS.h"
#include "task.h"

static bool audio_ready;
static bool audio_busy;

static bool audio_begin(bool require_ready)
{
    if (__get_IPSR() != 0U || xTaskGetSchedulerState() != taskSCHEDULER_RUNNING)
    {
        return false;
    }

    taskENTER_CRITICAL();
    const bool available = !audio_busy && (!require_ready || audio_ready);
    if (available)
    {
        audio_busy = true;
    }
    taskEXIT_CRITICAL();
    return available;
}

static void audio_end(void)
{
    taskENTER_CRITICAL();
    audio_busy = false;
    taskEXIT_CRITICAL();
}

bool audio_init(void)
{
    if (!audio_begin(false))
    {
        return false;
    }
    if (!audio_ready)
    {
        const HAL_StatusTypeDef status = wm8978_init();
        if (status != HAL_OK)
        {
            RLOG_ERR("Audio initialization failed (%u)", (unsigned)status);
            audio_end();
            return false;
        }
        audio_ready = true;
        RLOG_INFO("Audio ready: WM8978, I2S 48 kHz, speaker muted");
    }
    audio_end();
    return true;
}

bool audio_record(int16_t *pcm, size_t frames)
{
    if (pcm == NULL || frames == 0U || frames > SIZE_MAX / (2U * sizeof(*pcm)) ||
        !audio_begin(true))
    {
        return false;
    }
    const HAL_StatusTypeDef status = wm8978_record(pcm, frames);
    if (status != HAL_OK)
    {
        RLOG_ERR("Audio record failed (%u)", (unsigned)status);
    }
    audio_end();
    return status == HAL_OK;
}

bool audio_play(const int16_t *pcm, size_t frames)
{
    if (pcm == NULL || frames == 0U || frames > SIZE_MAX / (2U * sizeof(*pcm)) ||
        !audio_begin(true))
    {
        return false;
    }
    const HAL_StatusTypeDef status = wm8978_play(pcm, frames);
    if (status != HAL_OK)
    {
        RLOG_ERR("Audio play failed (%u)", (unsigned)status);
    }
    audio_end();
    return status == HAL_OK;
}

bool audio_record_blocks(audio_capture_sink_t sink, void *context, size_t max_frames)
{
    if (sink == NULL || max_frames == 0U || max_frames > SIZE_MAX / (2U * sizeof(int16_t)) ||
        !audio_begin(true))
    {
        return false;
    }
    const HAL_StatusTypeDef status = wm8978_record_blocks(sink, context, max_frames);
    if (status != HAL_OK)
    {
        RLOG_ERR("Audio record failed (%u)", (unsigned)status);
    }
    audio_end();
    return status == HAL_OK;
}

bool audio_play_blocks(audio_play_source_t source, void *context, size_t frames)
{
    if (source == NULL || frames == 0U || frames > SIZE_MAX / (2U * sizeof(int16_t)) ||
        !audio_begin(true))
    {
        return false;
    }
    const HAL_StatusTypeDef status = wm8978_play_blocks(source, context, frames);
    if (status != HAL_OK)
    {
        RLOG_ERR("Audio play failed (%u)", (unsigned)status);
    }
    audio_end();
    return status == HAL_OK;
}
