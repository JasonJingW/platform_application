#include "version_build.h"
#include "version.h"
#include <stddef.h>

/* UTF-8 box drawing characters are one cell wide in the console. */
static const char *const startup_banner[] = {
    u8"***************************************************************",
    u8"*                                                             *",
    u8"*                              /\\                             *",
    u8"*                     /\\      /  \\       /\\                   *",
    u8"*                    /  \\____/    \\_____/  \\                  *",
    u8"*                   /                       \\                 *",
    u8"*                                                             *",
    u8"*    ┌───╮  ──┬──  ┌───╮  ┌───╮  ╭───╮  ╭───╮  ╷   ╷          *",
    u8"*    │   │    │    │   │  │   │  │   │  │   │  │╲ ╱│          *",
    u8"*    ├───╯    │    │   │  ├───╯  │   │  ├───┤  │ ╵ │          *",
    u8"*    │  ╲     │    │   │  │  ╲   │   │  │   │  │   │          *",
    u8"*    ╵   ╲  ──┴──  └───╯  ╵   ╲  ╰───╯  ╵   ╵  ╵   ╵          *",
    u8"*                                                             *",
    u8"*                  R I D G E   &   R O A M                    *",
    u8"*                                                             *",
    u8"***************************************************************",
};

void version_print_banner(void (*write_line)(const char *line))
{
    for (size_t row = 0U; row < sizeof(startup_banner) / sizeof(startup_banner[0]); ++row)
    {
        write_line(startup_banner[row]);
    }
}

void version_print_info(void (*write_line)(const char *line))
{
    write_line("Project  " PROJECT_NAME);
    write_line("Version  " PROJECT_VERSION_STRING);
    write_line("Build    " PROJECT_BUILD_DATE " " PROJECT_BUILD_TIME);
    write_line("Commit");
    write_line("  Project Commit      " PROJECT_BUILD_COMMIT);
    write_line("  Common Commit       " PROJECT_BUILD_COMMON_COMMIT);
    write_line("  Application Commit  " PROJECT_BUILD_APPLICATION_COMMIT);
}
