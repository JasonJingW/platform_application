#include "task_manage.h"
#include "task_config.h"

/* Only the manager's reference is weak; the task implementation stays strong.
 * An omitted task source resolves to NULL with the ARM GCC/ELF linker. */
extern BaseType_t task_heartbeat_create(UBaseType_t priority) __attribute__((weak));
extern BaseType_t task_audio_create(UBaseType_t priority) __attribute__((weak));
extern BaseType_t task_network_create(UBaseType_t priority) __attribute__((weak));

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    RLOG_ERR("FreeRTOS heap allocation failed");
    for (;;)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    /* Avoid formatting or dereferencing task data on a possibly damaged stack. */
    (void)task_name;
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

static void task_create_optional(BaseType_t (*create)(UBaseType_t), UBaseType_t priority)
{
    if (create == NULL)
    {
        RLOG_WAR("Optional task not linked, skipped");
        return;
    }
    if (create(priority) != pdPASS)
    {
        vApplicationMallocFailedHook();
    }
}

void task_start(void)
{
    task_create_optional(task_heartbeat_create, HEARTBEAT_TASK_PRIORITY);
    task_create_optional(task_audio_create, AUDIO_TASK_PRIORITY);
    task_create_optional(task_network_create, NETWORK_TASK_PRIORITY);

    vTaskStartScheduler();
    /* The scheduler must not return; do not use HAL_Delay after this failure. */
    taskDISABLE_INTERRUPTS();
    RLOG_ERR("FreeRTOS scheduler returned unexpectedly");
    for (;;)
    {
    }
}
