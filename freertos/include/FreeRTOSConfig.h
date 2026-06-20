#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ─── Scheduler ──────────────────────────────────────────────────────────── */
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TICKLESS_IDLE                  0
#define configCPU_CLOCK_HZ                       133000000
#define configTICK_RATE_HZ                       1000
#define configMAX_PRIORITIES                     8
#define configMINIMAL_STACK_SIZE                 256
#define configMAX_TASK_NAME_LEN                  16
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1

/* ─── Memory ─────────────────────────────────────────────────────────────── */
#define configTOTAL_HEAP_SIZE                    (64 * 1024)
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1

/* ─── Mutexes & Semaphores ───────────────────────────────────────────────── */
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1

/* ─── Hooks ──────────────────────────────────────────────────────────────── */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             0
#define configCHECK_FOR_STACK_OVERFLOW           2

/* ─── Software timers ────────────────────────────────────────────────────── */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                 (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                  10
#define configTIMER_TASK_STACK_DEPTH              512

/* ─ Co-routines ────────────────────────────────────────────────────────────── */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES           2

/* ─── INCLUDE functions ──────────────────────────────────────────────────── */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xTaskAbortDelay                  0
#define INCLUDE_xTaskGetHandle                   0

/* ─── RP2040 port specifics ──────────────────────────────────────────────── */
#define configKERNEL_INTERRUPT_PRIORITY          (255)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      (191)



/* ─── Assert ─────────────────────────────────────────────────────────────── */
#define configASSERT(x) do { if (!(x)) { __asm volatile("bkpt #0"); } } while (0)

/* ─ SMP ───────────────────────────────────────────────────────────────────── */
#define configNUMBER_OF_CORES                    1
#define configRUN_MULTIPLE_PRIORITIES             0

#endif /* FREERTOS_CONFIG_H */
