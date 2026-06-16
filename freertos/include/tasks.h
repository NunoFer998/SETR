#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

void task_read_input(void *params);
void task_update_square(void *params);
void task_display(void *params);
void task_audio(void *params);
void audio_task_request_drop_from_isr(void);

/* Audio sporadic server state - for monitoring/debugging */
extern volatile uint32_t execution_budget_us;

#endif /* TASKS_H */