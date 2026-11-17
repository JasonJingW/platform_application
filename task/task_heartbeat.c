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
    if (HEARTBEAT_TASK_START_DELAY_MS > 0U)
    {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_TASK_START_DELAY_MS));
    }
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;)
    {
        Board_LEDToggle(0);
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS));
    }
}
