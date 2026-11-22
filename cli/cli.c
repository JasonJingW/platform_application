#include "cli.h"
#include "project_config.h"
#include "bsp_usart.h"
#include "cli_audio.h"

#define CLI_LINE_SIZE (64U)
#define CLI_HISTORY_SIZE (8U)
#define CLI_PROMPT "> "
#define CLI_SECTION_INDENT "  "
#define CLI_DETAIL_INDENT CLI_SECTION_INDENT CLI_SECTION_INDENT
#define CLI_HELP_COMMAND_WIDTH (14)

typedef struct
{
    const char *name;
    const char *description;
    void (*handler)(void);
} cli_command_t;

typedef enum
{
    CLI_ESCAPE_NONE,
    CLI_ESCAPE_START,
    CLI_ESCAPE_CSI,
    CLI_ESCAPE_SS3,
} cli_escape_t;

typedef struct
{
    char line[CLI_LINE_SIZE];
    char history[CLI_HISTORY_SIZE][CLI_LINE_SIZE];
    char draft[CLI_LINE_SIZE];
    size_t length;
    size_t cursor;
    size_t history_count;
    size_t history_next;
    size_t history_offset;
    size_t draft_cursor;
    size_t completion_prefix_end;
    size_t completion_next;
    cli_escape_t escape;
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
    {"audio tone", "Play a short speaker test tone", cli_audio_tone},
    {"audio loopback", "Record the onboard mic and play it back", cli_audio_loopback},
    {"audio replay", "Play the most recent mic recording", cli_audio_replay},
};

#define CLI_COMMAND_COUNT (sizeof(cli_commands) / sizeof(cli_commands[0]))

static void cli_write(const char *text)
{
    bsp_usart_write((const uint8_t *)text, (uint16_t)strlen(text));
}

static void cli_redraw(cli_input_t *input)
{
    input->line[input->length] = '\0';
    cli_write("\r" CLI_PROMPT);
    cli_write(input->line);
    cli_write("\x1b[K");
    if (input->cursor < input->length)
    {
        char move[16];
        snprintf(move, sizeof(move), "\x1b[%uD", (unsigned)(input->length - input->cursor));
        cli_write(move);
    }
}

static size_t cli_history_index(const cli_input_t *input, size_t offset)
{
    return (input->history_next + CLI_HISTORY_SIZE - offset) % CLI_HISTORY_SIZE;
}

static void cli_history_add(cli_input_t *input, const char *command)
{
    if (input->history_count != 0U &&
        strcmp(input->history[cli_history_index(input, 1U)], command) == 0)
    {
        return;
    }
    strcpy(input->history[input->history_next], command);
    input->history_next = (input->history_next + 1U) % CLI_HISTORY_SIZE;
    if (input->history_count < CLI_HISTORY_SIZE)
    {
        ++input->history_count;
    }
}

static void cli_history_move(cli_input_t *input, bool older)
{
    if (older)
    {
        if (input->history_offset == input->history_count)
        {
            return;
        }
        if (input->history_offset == 0U)
        {
            memcpy(input->draft, input->line, input->length);
            input->draft[input->length] = '\0';
            input->draft_cursor = input->cursor;
        }
        ++input->history_offset;
    }
    else
    {
        if (input->history_offset == 0U)
        {
            return;
        }
        --input->history_offset;
    }

    const bool restore_draft = input->history_offset == 0U;
    const char *line = restore_draft
                           ? input->draft
                           : input->history[cli_history_index(input, input->history_offset)];
    strcpy(input->line, line);
    input->length = strlen(line);
    input->cursor = restore_draft ? input->draft_cursor : input->length;
    cli_redraw(input);
}

static bool cli_process_escape(cli_input_t *input, uint8_t byte)
{
    if (byte == 0x1BU)
    {
        input->escape = CLI_ESCAPE_START;
        return true;
    }
    if (input->escape == CLI_ESCAPE_START)
    {
        input->escape = CLI_ESCAPE_NONE;
        if (byte == '[' || byte == 'O')
        {
            input->escape = byte == '[' ? CLI_ESCAPE_CSI : CLI_ESCAPE_SS3;
            return true;
        }
    }
    if (input->escape == CLI_ESCAPE_NONE)
    {
        return false;
    }
    if (input->escape == CLI_ESCAPE_CSI && byte >= 0x20U && byte <= 0x3FU)
    {
        return true;
    }

    input->escape = CLI_ESCAPE_NONE;
    switch (byte)
    {
    case 'A':
    case 'B':
        cli_history_move(input, byte == 'A');
        break;
    case 'C':
        if (input->cursor < input->length)
        {
            ++input->cursor;
            cli_write("\x1b[C");
        }
        break;
    case 'D':
        if (input->cursor > 0U)
        {
            --input->cursor;
            cli_write("\b");
        }
        break;
    default:
        break;
    }
    return true;
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

void cli_write_section_line(const char *line)
{
    cli_write(CLI_SECTION_INDENT);
    cli_write_line(line);
}

void cli_write_detail_line(const char *line)
{
    cli_write(CLI_DETAIL_INDENT);
    cli_write_line(line);
}

static void cli_version(void)
{
    cli_write_section_line("APP Firmware");
    version_print_info(cli_write_detail_line);
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
    cli_history_add(input, command);
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

static size_t cli_completion_common_length(size_t first, size_t prefix_length, size_t capacity)
{
    const char *name = cli_commands[first].name;
    size_t length = strlen(name);
    for (size_t i = first + 1U; i < CLI_COMMAND_COUNT; ++i)
    {
        const char *other = cli_commands[i].name;
        if (strncmp(name, other, prefix_length) != 0 || strlen(other) >= capacity)
        {
            continue;
        }
        size_t shared = prefix_length;
        while (shared < length && name[shared] == other[shared])
        {
            ++shared;
        }
        length = shared;
    }
    return length;
}

static void cli_complete(cli_input_t *input)
{
    input->line[input->length] = '\0';
    if (input->completion_next == 0U)
    {
        input->completion_prefix_end = input->length;
    }

    const size_t start = strspn(input->line, " ");
    const char *prefix = input->line + start;
    const size_t prefix_length = input->completion_prefix_end - start;
    for (size_t i = 0U; i < CLI_COMMAND_COUNT; ++i)
    {
        const size_t index = (input->completion_next + i) % CLI_COMMAND_COUNT;
        const char *name = cli_commands[index].name;
        if (strncmp(name, prefix, prefix_length) != 0 ||
            start + strlen(name) >= sizeof(input->line))
        {
            continue;
        }

        const char *suffix = name + prefix_length;
        char *shown = input->line + input->completion_prefix_end;
        if (input->completion_next == 0U)
        {
            const size_t common =
                cli_completion_common_length(index, prefix_length, sizeof(input->line) - start);
            if (common > prefix_length)
            {
                const size_t extra = common - prefix_length;
                memcpy(shown, suffix, extra);
                shown[extra] = '\0';
                input->length += extra;
                cli_write(shown);
                return;
            }
        }

        /* Keep the common part of the line when replacing the previous match. */
        input->completion_next = index + 1U;
        if (strcmp(shown, suffix) != 0)
        {
            while (input->length > input->completion_prefix_end)
            {
                cli_write("\b \b");
                --input->length;
            }
            const size_t length = strlen(suffix);
            memcpy(shown, suffix, length + 1U);
            input->length += length;
            cli_write(suffix);
        }
        return;
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

    if (byte != '\t')
    {
        input->completion_next = 0U;
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
        input->cursor = 0U;
        input->history_offset = 0U;
        input->escape = CLI_ESCAPE_NONE;
        input->discard = false;
        cli_write(CLI_PROMPT);
        return;
    }

    if (input->discard || cli_process_escape(input, byte))
    {
        return;
    }
    if (byte == '\t')
    {
        if (input->cursor < input->length)
        {
            input->cursor = input->length;
            cli_redraw(input);
        }
        cli_complete(input);
        input->cursor = input->length;
        return;
    }
    if (byte == '\b' || byte == 0x7FU)
    {
        if (input->cursor > 0U)
        {
            const bool at_end = input->cursor == input->length;
            --input->cursor;
            --input->length;
            memmove(input->line + input->cursor, input->line + input->cursor + 1U,
                    input->length - input->cursor);
            if (at_end)
            {
                cli_write("\b \b");
            }
            else
            {
                cli_redraw(input);
            }
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
        const bool at_end = input->cursor == input->length;
        memmove(input->line + input->cursor + 1U, input->line + input->cursor,
                input->length - input->cursor);
        input->line[input->cursor++] = (char)byte;
        ++input->length;
        if (at_end)
        {
            bsp_usart_write(&byte, 1U);
        }
        else
        {
            cli_redraw(input);
        }
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
    static cli_input_t input;
    memset(&input, 0, sizeof(input));
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
