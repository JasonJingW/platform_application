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
    if (HEARTBEAT_TASK_START_DELAY_MS > 0U)
    {
        vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_TASK_START_DELAY_MS));
    }
    TickType_t last_wake_time = xTaskGetTickCount();
    TickType_t last_led_toggle = last_wake_time;
    TickType_t last_led1_toggle = last_wake_time;
    Board_LEDToggle(0);
    Board_LEDToggle(1);
    for (;;)
    {
        xTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(HEARTBEAT_TASK_PERIOD_MS));
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_led_toggle) >= pdMS_TO_TICKS(HEARTBEAT_LED_TOGGLE_INTERVAL_MS))
        {
            Board_LEDToggle(0);
            last_led_toggle = now;
        }
        if ((TickType_t)(now - last_led1_toggle) >= pdMS_TO_TICKS(HEARTBEAT_LED1_TOGGLE_INTERVAL_MS))
        {
            Board_LEDToggle(1);
            last_led1_toggle = now;
        }
    }
}
