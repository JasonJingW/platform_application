#include "project_config.h"
#include "board.h"
#include "task/task_manage.h"

int main(void)
{
    hardware_init();
    segger_rtt_init();
    Board_Init();
    Board_ClockInit();
    RLOG_INFO("F767 FreeRTOS, CPU=%u Hz", (unsigned)SystemCoreClock);

    task_start();
}
