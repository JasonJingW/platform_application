#include "task_audio.h"
#include "../audio/audio.h"

static void task_audio_process(void *argument)
{
    (void)argument;
    audio_init();
    vTaskDelete(NULL);
}

BaseType_t task_audio_create(UBaseType_t priority)
{
    return xTaskCreate(task_audio_process, AUDIO_TASK_NAME, AUDIO_TASK_STACK_DEPTH, NULL, priority,
                       NULL);
}
