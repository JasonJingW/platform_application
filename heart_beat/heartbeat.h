#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Heartbeat initialization hook. */
    void heartbeat_init(void);

    /* Run one heartbeat cycle using the task period in milliseconds. */
    void heartbeat_process(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif
