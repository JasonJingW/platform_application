#include "heartbeat.h"
#include "bsp_gpio.h"

#define HEARTBEAT_LED_TOGGLE_INTERVAL_MS (500U)
#define HEARTBEAT_LED1_TOGGLE_INTERVAL_MS (1000U)

static void system_runningLed(uint32_t period)
{
    static uint64_t led_elapsed_ms;
    static uint64_t led1_elapsed_ms;

    led_elapsed_ms += period;
    led1_elapsed_ms += period;
    if (led_elapsed_ms >= HEARTBEAT_LED_TOGGLE_INTERVAL_MS)
    {
        bsp_gpio_toggle(MCU_LED);
        led_elapsed_ms %= HEARTBEAT_LED_TOGGLE_INTERVAL_MS;
    }
    if (led1_elapsed_ms >= HEARTBEAT_LED1_TOGGLE_INTERVAL_MS)
    {
        bsp_gpio_toggle(MCU_LED1);
        led1_elapsed_ms %= HEARTBEAT_LED1_TOGGLE_INTERVAL_MS;
    }
}

void heartbeat_init(void)
{
}

void heartbeat_process(uint32_t period)
{
    system_runningLed(period);
}
