#include "project_config.h"
#include "../cli/cli.h"

#define CLI_TASK_STACK_DEPTH (512U)

static void task_cli_process(void *argument)
{
    (void)argument;
    cli_run();
    vTaskDelete(NULL);
}

BaseType_t task_cli_create(UBaseType_t priority)
{
    return xTaskCreate(task_cli_process, "cli", CLI_TASK_STACK_DEPTH, NULL, priority, NULL);
}
