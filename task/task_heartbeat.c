#include "task_heartbeat.h"
#include "../heartbeat.h"

static void task_heartbeat_process(void *argument)
{
    (void)argument;
    TickType_t last_wake_time = xTaskGetTickCount();
    heartbeat_init();
    for (;;)
    {
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS));
        heartbeat_process(HEARTBEAT_TASK_PERIOD_MS);
    }
}

BaseType_t task_heartbeat_create(UBaseType_t priority)
{
    return xTaskCreate(task_heartbeat_process, HEARTBEAT_TASK_NAME, HEARTBEAT_TASK_STACK_DEPTH,
                       NULL, priority, NULL);
}
