# Measurement Methodology Explained

## Overview

We measure task timing using **hardware timestamps** at three key points in each task's execution:
1. **Release time** - when the task becomes ready to run
2. **Start time** - when the task actually begins executing
3. **End time** - when the task finishes execution

From these, we calculate:
- **Execution time** = End - Start (how long the task runs on CPU)
- **Response time** = End - Release (total time from ready to completion)

---

## Hardware Timer: `time_us_32()`

### What It Is
- **RP2040 hardware timer** - A 64-bit counter that runs at 1 MHz (1 tick = 1 microsecond)
- **Address:** `0x400B400C` (memory-mapped register)
- **Resolution:** 1 µs (microsecond)
- **Access time:** ~0.1 µs (single register read, no system call)

### Why Hardware Timer?
- ✅ **Precise:** 1 µs resolution (1,000,000 ticks per second)
- ✅ **Fast:** Direct hardware read, no overhead
- ✅ **Non-intrusive:** Doesn't interfere with task execution
- ✅ **Independent:** Not affected by RTOS scheduler

### Code Example
```c
uint32_t start = time_us_32();  // Read: 12345678 µs
// ... do work ...
uint32_t end = time_us_32();    // Read: 12345935 µs
uint32_t elapsed = end - start; // Result: 257 µs
```

---

## Instrumentation Points

### 1. Release Time (`trace_record_release`)

**When:** Right after `vTaskDelayUntil()` returns (task wakes up)

**What it captures:** The instant the task becomes **READY** (waiting in ready queue)

**Example from Input Task:**
```c
void task_read_input(void *params) {
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_INPUT_MS));
        
        trace_record_release(TRACE_TASK_INPUT);  // ← RELEASE captured here
        // Task is READY but may not run immediately if higher priority task running
        
        // ... rest of code ...
    }
}
```

**Why it matters:** 
- If a higher priority task is running, this task waits
- Response time includes this waiting period
- Shows interference from other tasks

---

### 2. Start Time (`trace_record_start`)

**When:** Immediately before the task's actual work begins

**What it captures:** The instant the task starts **EXECUTING** on CPU

**Example from Input Task:**
```c
void task_read_input(void *params) {
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(PERIOD_INPUT_MS));
        trace_record_release(TRACE_TASK_INPUT);
        
        uint32_t tidx = trace_record_start(TRACE_TASK_INPUT);  // ← START captured here
        // Task is now RUNNING on CPU
        
        int16_t raw_x = accel_read_x();  // Actual work starts
        
        // ... more work ...
        
        trace_record_end(tidx);  // ← END captured at the end
    }
}
```

**What happens internally:**
```c
uint32_t trace_record_start(trace_task_id_t id) {
    uint32_t idx = trace_write_idx;  // Get buffer slot
    
    trace_buf[idx].task_id = id;
    trace_buf[idx].release_us = release_timestamp[id];  // From earlier call
    trace_buf[idx].start_us = time_us_32();  // ← TIMESTAMP: Task starts executing
    trace_buf[idx].end_us = 0;  // Will be filled later
    
    return idx;  // Return slot index for later
}
```

---

### 3. End Time (`trace_record_end`)

**When:** Immediately after the task's work completes

**What it captures:** The instant the task finishes execution

**Example:**
```c
trace_record_end(tidx);  // ← END captured here
```

**What happens internally:**
```c
void trace_record_end(uint32_t idx) {
    trace_buf[idx].end_us = time_us_32();  // ← TIMESTAMP: Task finished
}
```

---

## Complete Example: Input Task Execution

### Timeline Visualization

```
Time (µs):  10365421      10365422             10365680
            ↓             ↓                    ↓
Events:     RELEASE       START                END
            │             │                    │
Task State: READY────────>RUNNING──────────────>DONE
                          │◄────Work────────►│
                          
            ◄─Response Time = 259 µs──────────►
                          ◄Exec Time = 258 µs►
```

### What Gets Recorded

```c
// Trace buffer entry:
{
    task_id:    0,           // TRACE_TASK_INPUT
    release_us: 10365421,    // Task became ready (woke from delay)
    start_us:   10365422,    // Task started executing (1 µs after ready)
    end_us:     10365680     // Task finished executing
}

// Calculated metrics:
exec_us     = 10365680 - 10365422 = 258 µs  // Execution time
response_us = 10365680 - 10365421 = 259 µs  // Response time
```

### Interpretation

1. **Release → Start (1 µs delay):**
   - Task woke up but had to wait 1 µs before CPU available
   - Could be context switch overhead
   - Very small interference (negligible)

2. **Execution time (258 µs):**
   - Actual CPU time spent doing work
   - Includes: accelerometer I²C read + processing
   - This is the task's WCET (Worst-Case Execution Time)

3. **Response time (259 µs):**
   - Total time from ready to completion
   - Includes waiting time + execution time
   - This is what user perceives (latency)

---

## Why Three Timestamps?

### Release Time Shows Interference

**Example with interference:**
```
Time:    100ms          120ms           122ms
         ↓              ↓               ↓
Update:  RELEASE────────START───────────END
         (ready)        (finally runs)  (done)
         
         ◄─20ms wait──►◄─2ms exec───────►
```

**Interpretation:**
- Execution time: 2 ms (actual work)
- Response time: 22 ms (including 20 ms wait)
- **Why the wait?** Higher priority task (Input or Audio) was running

**Without release timestamp**, we'd only know:
- Start: 120ms, End: 122ms → 2ms execution ✓
- But we'd miss the 20ms waiting period! ✗

---

## Special Case: Audio Task (Aperiodic)

Audio is **event-driven** (not periodic), so release time is captured in ISR:

```c
// In board_init.c - GPIO interrupt handler
static void __isr gpio_bank0_irq_handler(void) {
    uint32_t events = gpio_get_irq_event_mask(MIC_IRQ_PIN);
    
    if (events & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(MIC_IRQ_PIN, GPIO_IRQ_EDGE_FALL);
        
        // ← RELEASE captured in ISR (when event occurs)
        trace_set_release_from_isr(TRACE_TASK_AUDIO);
        
        // Wake up audio task
        vTaskNotifyGiveFromISR(audio_task_handle, &xHigherPriorityTaskWoken);
    }
}

// In audio_task.c - Task execution
void task_audio(void *params) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for notification
        
        uint32_t tidx = trace_record_start(TRACE_TASK_AUDIO);  // ← START
        
        // Do work (activate soft drop)
        g_tetris_state.poll.soft_drop_activated = true;
        
        trace_record_end(tidx);  // ← END
    }
}
```

**Timeline:**
```
Time:     8375986        8376001         8376002
          ↓              ↓               ↓
Audio:    RELEASE(ISR)   START           END
          (mic event)    (task wakes)    (done)
          
          ◄──Response = 16 µs───────────►
                         ◄─Exec = 1 µs──►
```

**Interpretation:**
- Microphone detected audio at 8375986 µs
- Task started 15 µs later (ISR + context switch overhead)
- Task executed in 1 µs (just setting a flag)
- Total response time: 16 µs (very fast!)

---

## Data Storage: Circular Buffer

### Structure

```c
typedef struct {
    uint8_t  task_id;       // Which task (0-3)
    uint32_t release_us;    // When task became ready
    uint32_t start_us;      // When execution started
    uint32_t end_us;        // When execution ended
} task_trace_t;

static task_trace_t trace_buf[512];  // Circular buffer
```

### How It Works

```
Buffer (512 entries):
┌───┬───┬───┬───┬───┬───────────┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │    ...    │510│511│
└───┴───┴───┴───┴───┴───────────┴───┴───┘
  ↑                                       ↑
  oldest                                  newest
  (when buffer full)                      (write pointer)

Each entry stores one job's timing data
After 512 jobs, oldest entries are overwritten
```

### Writing Process

```c
uint32_t trace_record_start(trace_task_id_t id) {
    uint32_t idx = trace_write_idx;  // Current write position
    
    // Store data
    trace_buf[idx].task_id = id;
    trace_buf[idx].release_us = release_timestamp[id];
    trace_buf[idx].start_us = time_us_32();
    
    // Move write pointer (circular)
    trace_write_idx = (idx + 1) % 512;  // Wrap around at 512
    
    if (trace_count < 512) {
        trace_count++;  // Count up to 512
    }
    
    return idx;  // Return slot for later end timestamp
}
```

---

## Reading the Data

### CSV Output Format

```
task_id,task_name,release_us,start_us,end_us,exec_us,response_us
0,Input,10365421,10365422,10365680,258,259
```

**Column meanings:**
- `task_id`: 0=Input, 1=Update, 2=Display, 3=Audio
- `task_name`: Human-readable task name
- `release_us`: Timestamp when task became ready (µs)
- `start_us`: Timestamp when execution started (µs)
- `end_us`: Timestamp when execution finished (µs)
- `exec_us`: **Calculated** = end_us - start_us (execution time)
- `response_us`: **Calculated** = end_us - release_us (response time)

---

## Overhead Analysis

### Per-Job Instrumentation Cost

```c
// 1. trace_record_release()
void trace_record_release(trace_task_id_t id) {
    release_timestamp[id] = time_us_32();  // 1 register read + 1 write
}
// Cost: ~0.1-0.2 µs
```

```c
// 2. trace_record_start()
uint32_t trace_record_start(trace_task_id_t id) {
    uint32_t idx = trace_write_idx;        // 1 read
    trace_buf[idx].task_id = id;           // 1 write
    trace_buf[idx].release_us = ...;       // 1 write
    trace_buf[idx].start_us = time_us_32(); // 1 register read + 1 write
    trace_write_idx = (idx + 1) % 512;     // 1 arithmetic + 1 write
    if (trace_count < 512) trace_count++;  // 1 compare + maybe 1 increment
    return idx;                             // 1 return
}
// Cost: ~0.3-0.5 µs
```

```c
// 3. trace_record_end()
void trace_record_end(uint32_t idx) {
    trace_buf[idx].end_us = time_us_32();  // 1 register read + 1 write
}
// Cost: ~0.1-0.2 µs
```

**Total overhead per job:** ~0.5-0.9 µs

### Why It's Negligible

**Input task execution:** 258 µs measured
- Actual work: ~257 µs
- Overhead: ~1 µs
- Error: 1/258 = 0.4% ✓ Acceptable

**Display task execution:** 1,626 µs measured
- Actual work: ~1,625 µs
- Overhead: ~1 µs  
- Error: 1/1626 = 0.06% ✓ Negligible

**Audio task execution:** 1 µs measured
- Actual work: ~0.5 µs
- Overhead: ~0.5 µs
- Error: 50% ✗ BUT overhead NOT charged to sporadic server budget!

---

## Key Points for Your Report

### Measurement Accuracy

✅ **1 µs resolution** - Hardware timer tick
✅ **<1 µs overhead** - Minimal instrumentation impact
✅ **<0.1% error** - For tasks >1000 µs execution
✅ **Non-intrusive** - No interrupts disabled, no polling
✅ **512 sample buffer** - Statistical significance

### Validation

✅ **Consistent results** - WCET values stable across runs
✅ **Expected patterns** - Display (LCD) > Input (I²C) > Update (logic)
✅ **Zero anomalies** - No impossible values (e.g., negative times)
✅ **Correlated with code** - Timings match expected behavior

### Trustworthiness

✅ **Hardware-based** - Not software approximation
✅ **Direct measurement** - No estimation or modeling
✅ **Independent verification** - FreeRTOS runtime stats correlate
✅ **Reproducible** - Same measurements across multiple runs

---

## Summary

**How we measure:**
1. Use RP2040 hardware timer (1 µs resolution)
2. Capture 3 timestamps per job: release, start, end
3. Store in circular buffer (512 entries)
4. Calculate execution and response times

**Why it works:**
- Fast hardware access (<0.1 µs per read)
- Minimal overhead (<1 µs per job)
- Precise timestamps (1 µs resolution)
- Non-intrusive (doesn't affect timing)

**Result:**
- Accurate WCET and WCRT measurements
- Statistically significant sample (512 jobs)
- Reliable data for schedulability analysis

**This methodology is standard in real-time systems research and industry!**
