#ifndef TASK_HEARTBEAT_H
#define TASK_HEARTBEAT_H

#include "project_config.h"

/* Heartbeat-owned settings; stack depth is measured in StackType_t elements. */
#define HEARTBEAT_TASK_NAME "heartbeat"
#define HEARTBEAT_TASK_STACK_DEPTH 512U
#define HEARTBEAT_TASK_PERIOD_MS 50U
#define HEARTBEAT_TASK_START_DELAY_MS 0U

#ifdef __cplusplus
extern "C" {
#endif

void heartBeat_process(void *argument);
BaseType_t heartBeat_create(UBaseType_t priority);

#ifdef __cplusplus
}
#endif

#endif
