#include "project_config.h"
#include "bsp_gpio.h"
#include "task/task_manage.h"

int main(void)
{
    hardware_init();
    segger_rtt_init();
    bsp_gpio_init();

    RLOG_PRINT("----------------------------------------");
    RLOG_PRINT("Project : %s", PROJECT_NAME);
    RLOG_PRINT("Version : %s", PROJECT_VERSION_STRING);
    RLOG_PRINT("Build   : %s %s", PROJECT_BUILD_DATE, PROJECT_BUILD_TIME);
    RLOG_PRINT("----------------------------------------");

    task_start();
}
