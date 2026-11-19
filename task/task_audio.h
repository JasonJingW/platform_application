#ifndef TASK_AUDIO_H
#define TASK_AUDIO_H

#include "project_config.h"

#define AUDIO_TASK_NAME "audio"
#define AUDIO_TASK_STACK_DEPTH (512U)

BaseType_t task_audio_create(UBaseType_t priority);

#endif
