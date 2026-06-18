# Accessing Trace Data Programmatically

## Overview

Instead of printing to USB serial (PuTTY), you can access the trace buffer directly in your code for analysis, logging to SD card, or custom processing.

## Available APIs

### 1. Get Direct Buffer Access

```c
uint32_t count, start_idx;
const task_trace_t *buffer = trace_get_buffer(&count, &start_idx);

// Now you have:
// - buffer: pointer to the circular buffer (512 entries)
// - count: number of valid entries (0-512)
// - start_idx: index of oldest entry (for circular access)
```

### 2. Get Individual Entries

```c
task_trace_t entry;
if (trace_get_entry(0, &entry)) {  // Get oldest entry
    uint32_t exec_us = entry.end_us - entry.start_us;
    uint32_t response_us = entry.end_us - entry.release_us;
    
    // Do something with the data...
}
```

### 3. Calculate Statistics

```c
task_stats_t stats;
trace_calculate_stats(TRACE_TASK_AUDIO, &stats);

// Now you have:
// - stats.count: number of jobs
// - stats.min_exec_us: best-case execution time
// - stats.max_exec_us: worst-case execution time (WCET)
// - stats.avg_exec_us: average execution time
// - stats.min_response_us: best-case response time
// - stats.max_response_us: worst-case response time (WCRT)
// - stats.avg_response_us: average response time
```

## Example 1: Custom Analysis Task

Instead of using `task_trace_dump`, create your own analysis:

```c
void task_custom_analysis(void *params) {
    (void)params;
    
    // Wait for data collection (10 seconds)
    vTaskDelay(pdMS_TO_TICKS(TRACE_COLLECTION_MS));
    
    printf("\n=== CUSTOM TIMING ANALYSIS ===\n\n");
    
    // Analyze each task
    const char *task_names[] = {"Input", "Update", "Display", "Audio"};
    
    for (int i = 0; i < TRACE_TASK_COUNT; i++) {
        task_stats_t stats;
        trace_calculate_stats(i, &stats);
        
        printf("Task: %s\n", task_names[i]);
        printf("  Jobs recorded: %lu\n", (unsigned long)stats.count);
        printf("  Execution Time:\n");
        printf("    Min:  %lu us\n", (unsigned long)stats.min_exec_us);
        printf("    Avg:  %lu us\n", (unsigned long)stats.avg_exec_us);
        printf("    Max:  %lu us (WCET)\n", (unsigned long)stats.max_exec_us);
        printf("  Response Time:\n");
        printf("    Min:  %lu us\n", (unsigned long)stats.min_response_us);
        printf("    Avg:  %lu us\n", (unsigned long)stats.avg_response_us);
        printf("    Max:  %lu us (WCRT)\n", (unsigned long)stats.max_response_us);
        printf("\n");
    }
    
    vTaskDelete(NULL);
}
```

## Example 2: Store to Array for Later Access

```c
// Define storage for processed data
typedef struct {
    uint32_t job_number;
    uint8_t task_id;
    uint32_t exec_us;
    uint32_t response_us;
} processed_trace_t;

#define MAX_PROCESSED 512
static processed_trace_t processed_data[MAX_PROCESSED];
static uint32_t processed_count = 0;

void process_trace_to_array(void) {
    uint32_t count, start;
    const task_trace_t *buffer = trace_get_buffer(&count, &start);
    
    processed_count = (count > MAX_PROCESSED) ? MAX_PROCESSED : count;
    
    for (uint32_t i = 0; i < processed_count; i++) {
        uint32_t idx = (start + i) % TRACE_BUFFER_SIZE;
        const task_trace_t *t = &buffer[idx];
        
        processed_data[i].job_number = i;
        processed_data[i].task_id = t->task_id;
        processed_data[i].exec_us = t->end_us - t->start_us;
        processed_data[i].response_us = t->end_us - t->release_us;
    }
    
    // Now processed_data[] contains all the results
    // You can access it anytime without re-reading the circular buffer
}

// Later, access the processed data:
void print_task_executions(uint8_t task_id) {
    printf("Jobs for task %d:\n", task_id);
    for (uint32_t i = 0; i < processed_count; i++) {
        if (processed_data[i].task_id == task_id) {
            printf("  Job %lu: exec=%lu us, response=%lu us\n",
                   (unsigned long)processed_data[i].job_number,
                   (unsigned long)processed_data[i].exec_us,
                   (unsigned long)processed_data[i].response_us);
        }
    }
}
```

## Example 3: Find Deadline Misses

```c
void check_deadline_misses(void) {
    // Define deadlines (in microseconds)
    const uint32_t deadlines_us[] = {
        20000,   // Input:   20ms
        33000,   // Update:  33ms
        33000,   // Display: 33ms
        1000     // Audio:   1ms (for responsiveness)
    };
    
    uint32_t count, start;
    const task_trace_t *buffer = trace_get_buffer(&count, &start);
    
    uint32_t misses[TRACE_TASK_COUNT] = {0};
    uint32_t total[TRACE_TASK_COUNT] = {0};
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (start + i) % TRACE_BUFFER_SIZE;
        const task_trace_t *t = &buffer[idx];
        
        if (t->task_id >= TRACE_TASK_COUNT) continue;
        
        uint32_t response_us = t->end_us - t->release_us;
        total[t->task_id]++;
        
        if (response_us > deadlines_us[t->task_id]) {
            misses[t->task_id]++;
        }
    }
    
    printf("\n=== DEADLINE ANALYSIS ===\n");
    const char *names[] = {"Input", "Update", "Display", "Audio"};
    for (int i = 0; i < TRACE_TASK_COUNT; i++) {
        printf("%s: %lu/%lu jobs missed deadline (%.2f%%)\n",
               names[i],
               (unsigned long)misses[i],
               (unsigned long)total[i],
               total[i] > 0 ? (100.0f * misses[i] / total[i]) : 0.0f);
    }
}
```

## Example 4: Export to Binary File (SD Card)

If you have an SD card interface:

```c
#include "ff.h"  // FatFS library

void export_trace_to_sd(void) {
    FIL file;
    UINT bytes_written;
    
    if (f_open(&file, "trace.bin", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        printf("Failed to open file\n");
        return;
    }
    
    uint32_t count, start;
    const task_trace_t *buffer = trace_get_buffer(&count, &start);
    
    // Write header
    f_write(&file, &count, sizeof(count), &bytes_written);
    
    // Write all entries in chronological order
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = (start + i) % TRACE_BUFFER_SIZE;
        f_write(&file, &buffer[idx], sizeof(task_trace_t), &bytes_written);
    }
    
    f_close(&file);
    printf("Exported %lu entries to trace.bin\n", (unsigned long)count);
}
```

## Example 5: Real-Time Monitoring (Non-Blocking)

Monitor trace data while system is still running:

```c
void task_realtime_monitor(void *params) {
    (void)params;
    uint32_t last_reported_count = 0;
    
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
        
        uint32_t count, start;
        trace_get_buffer(&count, &start);
        
        if (count > last_reported_count) {
            // Process new entries
            uint32_t new_entries = count - last_reported_count;
            
            printf("[MONITOR] %lu new trace entries (total: %lu)\n",
                   (unsigned long)new_entries,
                   (unsigned long)count);
            
            // Analyze recent audio task performance
            task_stats_t audio_stats;
            trace_calculate_stats(TRACE_TASK_AUDIO, &audio_stats);
            
            printf("  Audio WCET: %lu us\n", (unsigned long)audio_stats.max_exec_us);
            printf("  Audio WCRT: %lu us\n", (unsigned long)audio_stats.max_response_us);
            
            last_reported_count = count;
        }
        
        // Check if buffer is full
        if (count >= TRACE_BUFFER_SIZE) {
            printf("[MONITOR] Buffer full! Oldest entries being overwritten.\n");
        }
    }
}
```

## Integration in main.c

Replace or modify the dump task:

```c
// Option 1: Replace trace dump task
xTaskCreate(task_custom_analysis, "Analysis", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);

// Option 2: Add real-time monitoring alongside other tasks
xTaskCreate(task_realtime_monitor, "Monitor", 512, NULL, 1, NULL);

// Option 3: Process after collection period
void custom_processing(void) {
    vTaskDelay(pdMS_TO_TICKS(TRACE_COLLECTION_MS));
    
    process_trace_to_array();
    check_deadline_misses();
    export_trace_to_sd();
    
    // Now processed_data[] is available for the rest of the program
}
```

## Data Structure Reference

```c
// Single trace entry
typedef struct {
    uint8_t  task_id;       // 0=Input, 1=Update, 2=Display, 3=Audio
    uint32_t release_us;    // When task was released
    uint32_t start_us;      // When execution began
    uint32_t end_us;        // When execution finished
} task_trace_t;

// Calculated metrics:
// exec_us = end_us - start_us       (execution time)
// response_us = end_us - release_us (response time)

// Statistics structure
typedef struct {
    uint32_t count;              // Number of jobs
    uint32_t min_exec_us;        // Minimum execution time
    uint32_t max_exec_us;        // Maximum execution time (WCET)
    uint32_t avg_exec_us;        // Average execution time
    uint32_t min_response_us;    // Minimum response time
    uint32_t max_response_us;    // Maximum response time (WCRT)
    uint32_t avg_response_us;    // Average response time
} task_stats_t;
```

## Key Points

1. **Buffer is read-only:** Use `const task_trace_t*` to ensure you don't corrupt trace data
2. **Circular buffer:** Always use the start index provided by `trace_get_buffer()`
3. **Thread-safe reads:** Reading is safe while tracing is active (writes are atomic)
4. **Overhead:** Accessing the buffer has zero overhead on the traced tasks
5. **512 entry limit:** Buffer holds last 512 jobs, earlier ones are overwritten

## Summary

You now have three ways to access trace data:

1. **USB Serial (PuTTY):** `trace_dump_usb()` - convenient for quick analysis
2. **Direct buffer access:** `trace_get_buffer()` - full programmatic control
3. **Calculated statistics:** `trace_calculate_stats()` - ready-made analysis

Choose based on your needs! For your presentation, you can process the data in code and display only the key metrics.
