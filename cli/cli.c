#include "cli.h"
#include "project_config.h"
#include "bsp_usart.h"

#define CLI_LINE_SIZE (64U)
#define CLI_PROMPT "> "
#define CLI_SECTION_INDENT "  "
#define CLI_DETAIL_INDENT CLI_SECTION_INDENT CLI_SECTION_INDENT
#define CLI_HELP_COMMAND_WIDTH (10)

typedef struct
{
    const char *name;
    const char *description;
    void (*handler)(void);
} cli_command_t;

typedef struct
{
    char line[CLI_LINE_SIZE];
    size_t length;
    bool discard;
    bool skip_lf;
} cli_input_t;

static void cli_help(void);
static void cli_clear(void);
static void cli_version(void);
static void cli_status(void);

static const cli_command_t cli_commands[] = {
    {"help", "List commands", cli_help},
    {"clear", "Clear the terminal screen", cli_clear},
    {"version", "Show firmware and build information", cli_version},
    {"status", "Show system clock and RTOS tick count", cli_status},
};

#define CLI_COMMAND_COUNT (sizeof(cli_commands) / sizeof(cli_commands[0]))

static void cli_write(const char *text)
{
    bsp_usart_write((const uint8_t *)text, (uint16_t)strlen(text));
}

static void cli_help_row(const char *command, const char *description)
{
    char line[96];
    snprintf(line, sizeof(line), CLI_DETAIL_INDENT "%-*s  %s\r\n", CLI_HELP_COMMAND_WIDTH, command,
             description);
    cli_write(line);
}

static void cli_help(void)
{
    cli_write(CLI_SECTION_INDENT "Commands\r\n");
    for (size_t i = 0U; i < CLI_COMMAND_COUNT; ++i)
    {
        cli_help_row(cli_commands[i].name, cli_commands[i].description);
    }
}

static void cli_clear(void)
{
    cli_write("\x1b[2J\x1b[H");
}

static void cli_write_line(const char *line)
{
    cli_write(line);
    cli_write("\r\n");
}

static void cli_version_line(const char *line)
{
    cli_write(CLI_DETAIL_INDENT);
    cli_write_line(line);
}

static void cli_version(void)
{
    cli_write(CLI_SECTION_INDENT "APP Firmware\r\n");
    version_print_info(cli_version_line);
}

static void cli_status(void)
{
    char text[96];
    snprintf(text, sizeof(text),
             CLI_SECTION_INDENT "System status\r\n" CLI_DETAIL_INDENT
                                "System clock  %lu Hz\r\n" CLI_DETAIL_INDENT
                                "RTOS ticks    %lu\r\n",
             (unsigned long)SystemCoreClock, (unsigned long)xTaskGetTickCount());
    cli_write(text);
}

static void cli_execute(cli_input_t *input)
{
    size_t end = input->length;
    while (end > 0U && input->line[end - 1U] == ' ')
    {
        --end;
    }
    input->line[end] = '\0';

    const char *command = input->line + strspn(input->line, " ");
    if (*command == '\0')
    {
        return;
    }
    for (size_t i = 0U; i < CLI_COMMAND_COUNT; ++i)
    {
        if (strcmp(command, cli_commands[i].name) == 0)
        {
            cli_commands[i].handler();
            return;
        }
    }
    cli_write(CLI_SECTION_INDENT "Error\r\n" CLI_DETAIL_INDENT "Unknown command. Type help.\r\n");
}

static void cli_complete(cli_input_t *input)
{
    input->line[input->length] = '\0';
    const size_t start = strspn(input->line, " ");
    const char *prefix = input->line + start;
    const size_t prefix_length = input->length - start;
    const char *match = NULL;
    size_t matches = 0U;
    for (size_t i = 0U; i < CLI_COMMAND_COUNT; ++i)
    {
        if (strncmp(cli_commands[i].name, prefix, prefix_length) == 0)
        {
            match = cli_commands[i].name;
            ++matches;
        }
    }

    if (matches == 1U)
    {
        const char *suffix = match + prefix_length;
        const size_t length = strlen(suffix);
        if (length > 0U && length < sizeof(input->line) - input->length)
        {
            memcpy(input->line + input->length, suffix, length + 1U);
            input->length += length;
            cli_write(suffix);
        }
    }
    else if (matches > 1U)
    {
        cli_write("\r\n" CLI_SECTION_INDENT "Matches\r\n");
        for (size_t i = 0U; i < CLI_COMMAND_COUNT; ++i)
        {
            const char *name = cli_commands[i].name;
            if (strncmp(name, prefix, prefix_length) == 0)
            {
                cli_write(CLI_DETAIL_INDENT);
                cli_write(name);
                cli_write("\r\n");
            }
        }
        cli_write(CLI_PROMPT);
        if (input->length > 0U)
        {
            cli_write(input->line);
        }
    }
}

static void cli_process_byte(cli_input_t *input, uint8_t byte)
{
    const bool skip = input->skip_lf && byte == '\n';
    input->skip_lf = byte == '\r';
    if (skip)
    {
        return;
    }

    if (byte == '\r' || byte == '\n')
    {
        cli_write("\r\n");
        if (input->discard)
        {
            cli_write(CLI_SECTION_INDENT "Error\r\n" CLI_DETAIL_INDENT
                                         "Input too long or incomplete. Try again.\r\n");
        }
        else
        {
            cli_execute(input);
        }
        input->length = 0U;
        input->discard = false;
        cli_write(CLI_PROMPT);
        return;
    }

    if (input->discard)
    {
        return;
    }
    if (byte == '\t')
    {
        cli_complete(input);
        return;
    }
    if (byte == '\b' || byte == 0x7FU)
    {
        if (input->length > 0U)
        {
            --input->length;
            cli_write("\b \b");
        }
        return;
    }
    if (byte >= ' ' && byte <= '~')
    {
        if (input->length == sizeof(input->line) - 1U)
        {
            input->discard = true;
            return;
        }
        input->line[input->length++] = (char)byte;
        bsp_usart_write(&byte, 1U);
    }
}

void cli_run(void)
{
    const HAL_StatusTypeDef status = bsp_usart_init();
    if (status != HAL_OK)
    {
        RLOG_ERR("Serial CLI initialization failed (%u)", (unsigned)status);
        return;
    }

    cli_write("\r\n");
    version_print_banner(cli_write_line);
    cli_write("\r\n");
    cli_version();
    cli_write(CLI_PROMPT);
    cli_input_t input = {0};
    for (;;)
    {
        uint8_t byte;
        if (bsp_usart_read(&byte))
        {
            cli_process_byte(&input, byte);
        }
        else
        {
            input.discard = true;
            input.skip_lf = false;
        }
    }
}
