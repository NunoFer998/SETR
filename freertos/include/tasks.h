#ifndef TASKS_H
#define TASKS_H

void task_read_input(void *params);
void task_update_square(void *params);
void task_display(void *params);
void task_audio(void *params);
void audio_task_request_drop_from_isr(void);

#endif /* TASKS_H */