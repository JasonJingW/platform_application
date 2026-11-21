#include "project_config.h"
#include "bsp_gpio.h"
#include "task_manage.h"

static void boot_version_line(const char *line)
{
    (void)line;
    RLOG_PRINT("%s", line);
}

int main(void)
{
    hardware_init();
    segger_rtt_init();
    bsp_gpio_init();

    version_print_banner(boot_version_line);
    RLOG_PRINT("----------------------------------------");
    version_print_info(boot_version_line);
    RLOG_PRINT("----------------------------------------");

    task_start();
}
