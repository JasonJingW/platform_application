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
    RLOG_INFO("F767 application, CPU=%u Hz", (unsigned)SystemCoreClock);

#if PROJECT_USE_FREERTOS
    task_start();
#else
    for (;;) { __WFI(); }
#endif
}
