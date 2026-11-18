#ifndef TASK_HEARTBEAT_H
#define TASK_HEARTBEAT_H

#include "project_config.h"

#define HEARTBEAT_TASK_NAME "heartbeat"
#define HEARTBEAT_TASK_STACK_DEPTH (512U)
#define HEARTBEAT_TASK_PERIOD_MS (50U)

#ifdef __cplusplus
extern "C"
{
#endif

    BaseType_t task_heartbeat_create(UBaseType_t priority);

#ifdef __cplusplus
}
#endif

#endif
