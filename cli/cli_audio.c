#include "cli_audio.h"
#include "cli.h"
#include "../audio/audio.h"
#include "project_config.h"
#include "bsp_usart.h"

#define CLI_AUDIO_RATE (48000U)
#define CLI_AUDIO_CHANNELS (2U)
#define CLI_AUDIO_TONE_FRAMES (CLI_AUDIO_RATE / 2U)
#define CLI_AUDIO_WARMUP_FRAMES (CLI_AUDIO_RATE / 50U)
#define CLI_AUDIO_FADE_FRAMES (CLI_AUDIO_RATE / 100U)
#define CLI_AUDIO_PEAK_LIMIT (8192)
#define CLI_AUDIO_REPLAY_GAIN (16)
#define CLI_AUDIO_DECIMATION (2U)
#define CLI_AUDIO_RECORD_SECONDS (5U)
#define CLI_AUDIO_RECORD_FRAMES (CLI_AUDIO_RATE * CLI_AUDIO_RECORD_SECONDS)

static int16_t cli_audio_pcm[CLI_AUDIO_RECORD_FRAMES / CLI_AUDIO_DECIMATION];

typedef struct
{
    size_t warmup;
    size_t samples;
    size_t playback_frame;
    int32_t peak;
    int32_t replay_peak;
} cli_audio_recording_t;

static cli_audio_recording_t cli_audio_recording;

/* One 1 kHz cycle at 48 kHz, with a peak of 25% full scale. */
static const int16_t cli_audio_sine[] = {
    0,     1069,  2120,  3135,  4096,  4987,  5793,  6499,  7094,  7568,  7913,  8122,
    8192,  8122,  7913,  7568,  7094,  6499,  5793,  4987,  4096,  3135,  2120,  1069,
    0,     -1069, -2120, -3135, -4096, -4987, -5793, -6499, -7094, -7568, -7913, -8122,
    -8192, -8122, -7913, -7568, -7094, -6499, -5793, -4987, -4096, -3135, -2120, -1069,
};

static void cli_audio_report(const char *line)
{
    cli_write_detail_line(line);
    RLOG_INFO("Audio test: %s", line);
}

static int16_t cli_audio_fade(int32_t sample, size_t frame, size_t frames)
{
    const size_t remaining = frames - 1U - frame;
    const size_t edge = frame < remaining ? frame : remaining;
    if (edge < CLI_AUDIO_FADE_FRAMES)
    {
        sample = sample * (int32_t)edge / (int32_t)CLI_AUDIO_FADE_FRAMES;
    }
    return (int16_t)sample;
}

static void cli_audio_fill_tone(void *context, int16_t *pcm, size_t frames)
{
    size_t *position = context;
    for (size_t i = 0U; i < frames; ++i)
    {
        const size_t frame = (*position)++;
        const size_t phase = frame % (sizeof(cli_audio_sine) / sizeof(cli_audio_sine[0]));
        /* Raise the test tone to 37.5% full scale before applying the fade. */
        const int32_t level = (int32_t)cli_audio_sine[phase] * 3 / 2;
        const int16_t sample = cli_audio_fade(level, frame, CLI_AUDIO_TONE_FRAMES);
        pcm[i * CLI_AUDIO_CHANNELS] = sample;
        pcm[i * CLI_AUDIO_CHANNELS + 1U] = sample;
    }
}

void cli_audio_tone(void)
{
    cli_write_section_line("Audio tone");
    if (!audio_init())
    {
        cli_audio_report("Codec unavailable. Check RTT for details.");
        return;
    }

    cli_audio_report("Speaker  Playing 1 kHz for 500 ms");
    size_t position = 0U;
    const bool played = audio_play_blocks(cli_audio_fill_tone, &position, CLI_AUDIO_TONE_FRAMES);
    cli_audio_report(played ? "Speaker  Playback complete (24000 frames)"
                            : "Speaker  Playback failed");
}

static bool cli_audio_stop_requested(void)
{
    uint8_t key;
    while (bsp_usart_try_read(&key))
    {
        if (key == ' ')
        {
            return true;
        }
    }
    return false;
}

static bool cli_audio_capture(void *context, const int16_t *pcm, size_t frames)
{
    cli_audio_recording_t *recording = context;
    const size_t skip = frames < recording->warmup ? frames : recording->warmup;
    recording->warmup -= skip;

    /* Store the board mic's left channel at 24 kHz, averaging adjacent frames. */
    for (size_t i = skip; i + 1U < frames; i += CLI_AUDIO_DECIMATION)
    {
        const int32_t first = pcm[i * CLI_AUDIO_CHANNELS];
        const int32_t second = pcm[(i + 1U) * CLI_AUDIO_CHANNELS];
        const int16_t sample = (int16_t)((first + second) / 2);
        cli_audio_pcm[recording->samples++] = sample;
        const int32_t magnitude = abs((int)sample);
        if (magnitude > recording->peak)
        {
            recording->peak = magnitude;
        }
    }
    return !cli_audio_stop_requested();
}

static void cli_audio_fill_replay(void *context, int16_t *pcm, size_t frames)
{
    cli_audio_recording_t *recording = context;
    const size_t total_frames = recording->samples * CLI_AUDIO_DECIMATION;
    for (size_t i = 0U; i < frames; ++i)
    {
        const size_t frame = recording->playback_frame++;
        const size_t sample_index = frame / CLI_AUDIO_DECIMATION;
        int32_t sample = cli_audio_pcm[sample_index];
        if ((frame % CLI_AUDIO_DECIMATION) != 0U && sample_index + 1U < recording->samples)
        {
            sample = (sample + cli_audio_pcm[sample_index + 1U]) / 2;
        }
        sample = sample * recording->replay_peak / recording->peak;
        const int16_t output = cli_audio_fade(sample, frame, total_frames);
        pcm[i * CLI_AUDIO_CHANNELS] = output;
        pcm[i * CLI_AUDIO_CHANNELS + 1U] = output;
    }
}

static void cli_audio_play_recording(void)
{
    cli_audio_recording_t *recording = &cli_audio_recording;
    if (recording->samples == 0U || recording->peak == 0)
    {
        cli_audio_report("No recording. Run audio loopback first.");
        return;
    }

    const int32_t boosted_peak = recording->peak * CLI_AUDIO_REPLAY_GAIN;
    recording->replay_peak =
        boosted_peak < CLI_AUDIO_PEAK_LIMIT ? boosted_peak : CLI_AUDIO_PEAK_LIMIT;
    recording->playback_frame = 0U;
    const size_t frames = recording->samples * CLI_AUDIO_DECIMATION;
    char line[96];
    snprintf(line, sizeof(line), "Speaker  Playing recording (peak %ld -> %ld)",
             (long)recording->peak, (long)recording->replay_peak);
    cli_audio_report(line);
    const bool played = audio_play_blocks(cli_audio_fill_replay, recording, frames);
    snprintf(line, sizeof(line), "Speaker  Playback complete (%lu frames)", (unsigned long)frames);
    cli_audio_report(played ? line : "Speaker  Playback failed");
}

void cli_audio_replay(void)
{
    cli_write_section_line("Audio replay");
    cli_audio_play_recording();
}

static void cli_audio_record(void)
{
    if (!audio_init())
    {
        cli_audio_report("Codec unavailable. Check RTT for details.");
        return;
    }

    char line[96];
    snprintf(line, sizeof(line), "Mic      Recording. Space stops (max %u s).",
             CLI_AUDIO_RECORD_SECONDS);
    cli_audio_report(line);
    cli_audio_recording = (cli_audio_recording_t){
        .warmup = CLI_AUDIO_WARMUP_FRAMES,
    };
    if (!audio_record_blocks(cli_audio_capture, &cli_audio_recording,
                             CLI_AUDIO_WARMUP_FRAMES + CLI_AUDIO_RECORD_FRAMES))
    {
        cli_audio_recording.samples = 0U;
        cli_audio_report("Mic      Recording failed");
        return;
    }

    const size_t frames = cli_audio_recording.samples * CLI_AUDIO_DECIMATION;
    snprintf(line, sizeof(line), "Mic      %s, %lu ms, peak %ld",
             frames == CLI_AUDIO_RECORD_FRAMES ? "Time limit reached" : "Stopped",
             (unsigned long)(frames * 1000U / CLI_AUDIO_RATE), (long)cli_audio_recording.peak);
    cli_audio_report(line);

    cli_audio_play_recording();
}

void cli_audio_loopback(void)
{
    cli_write_section_line("Audio loopback");
    cli_write_detail_line("Press Space to start, Ctrl+C to cancel.");
    for (;;)
    {
        uint8_t key;
        if (!bsp_usart_read(&key) || key == 0x03U)
        {
            cli_write_detail_line("Cancelled");
            return;
        }
        if (key == ' ')
        {
            cli_audio_record();
            return;
        }
    }
}
