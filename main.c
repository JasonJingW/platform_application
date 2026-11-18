#include "project_config.h"
#include "board.h"
#if PROJECT_USE_FREERTOS
#include "task/task_manage.h"
#endif

int main(void)
{
    hardware_init();
    segger_rtt_init();
    Board_Init();
    Board_ClockInit();
    RLOG_PRINT("----------------------------------------");
    RLOG_PRINT("Project : %s", PROJECT_NAME);
    RLOG_PRINT("Version : %s", PROJECT_VERSION_STRING);
    RLOG_PRINT("Build   : %s %s", PROJECT_BUILD_DATE, PROJECT_BUILD_TIME);
    RLOG_PRINT("----------------------------------------");

#if PROJECT_USE_FREERTOS
    task_start();
#else
    for (;;) { __WFI(); }
#endif
}
