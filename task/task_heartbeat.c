#include "task_heartbeat.h"
#include "board.h"

BaseType_t heartBeat_create(UBaseType_t priority)
{
    return xTaskCreate(heartBeat_process, HEARTBEAT_TASK_NAME, HEARTBEAT_TASK_STACK_DEPTH, NULL,
                       priority, NULL);
}

void heartBeat_process(void *argument)
{
    (void)argument;
    RLOG_INFO("FreeRTOS heartbeat task started");
    for (;;)
    {
        Board_LEDToggle(0);
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS));
    }
}
