#include "audio.h"
#include "project_config.h"
#include "es8388.h"
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
        const HAL_StatusTypeDef status = es8388_init();
        if (status != HAL_OK)
        {
            RLOG_ERR("Audio initialization failed (%u)", (unsigned)status);
            audio_end();
            return false;
        }
        audio_ready = true;
        RLOG_INFO("Audio initialized, output muted");
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
    const HAL_StatusTypeDef status = es8388_record(pcm, frames);
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
    const HAL_StatusTypeDef status = es8388_play(pcm, frames);
    audio_end();
    return status == HAL_OK;
}
