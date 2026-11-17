#ifndef TASK_MANAGE_H
#define TASK_MANAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Create application tasks and start scheduling. Call once after initialization.
 * Does not return, including on task creation or scheduler startup failure. */
void task_start(void);

#ifdef __cplusplus
}
#endif

#endif
