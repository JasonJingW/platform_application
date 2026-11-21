#include "task_network.h"
#include "../network/network.h"

#define NETWORK_TASK_NAME "network"
#define NETWORK_TASK_STACK_DEPTH (768U)
#define NETWORK_TASK_PERIOD_MS (1U)
#define NETWORK_TASK_RETRY_MS (1000U)

static void task_network_process(void *argument)
{
    (void)argument;
    if (!network_init())
    {
        RLOG_ERR("Ethernet initialization failed, retrying");
        do
        {
            vTaskDelay(pdMS_TO_TICKS(NETWORK_TASK_RETRY_MS));
        } while (!network_init());
    }

    TickType_t last_wake = xTaskGetTickCount();
    for (;;)
    {
        network_process();
        xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(NETWORK_TASK_PERIOD_MS));
    }
}

BaseType_t task_network_create(UBaseType_t priority)
{
    return xTaskCreate(task_network_process, NETWORK_TASK_NAME, NETWORK_TASK_STACK_DEPTH, NULL,
                       priority, NULL);
}
