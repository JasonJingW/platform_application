#ifndef CLI_H
#define CLI_H

/* Run the serial console in a FreeRTOS task. Returns if initialization fails. */
void cli_run(void);

/* Output helpers for commands running in the CLI task. */
void cli_write_section_line(const char *line);
void cli_write_detail_line(const char *line);

#endif
