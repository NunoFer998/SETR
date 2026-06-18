# Critical Instant Test Guide

## What is a Critical Instant?

A **critical instant** is the worst-case scenario in real-time scheduling where **all periodic tasks become ready at exactly the same time** (t=0). This creates maximum interference and gives us the **absolute worst-case response time (WCRT)** for each task.

According to real-time scheduling theory (Liu & Layland, 1973), the worst-case response time for any task occurs when:
1. The task is released
2. All higher-priority tasks are also released at the same instant
3. All lower-priority tasks that started before this task must complete

This test validates that even under maximum interference, all tasks still meet their deadlines.

## Test Setup

The test is already implemented in:
- `src/critical_instant_test.c` - Synchronized task versions
- `include/critical_instant_test.h` - Header file
- `src/main.c` - Switch between normal/test mode

### How It Works

1. **Synchronization Barrier**: All tasks wait at a semaphore barrier
2. **Coordinator Task**: Waits for all tasks to be ready, then releases them simultaneously
3. **Trace Recording**: Each task records its release, start, and end timestamps
4. **Result Analysis**: Coordinator dumps trace data and calculates WCRT

## Running the Test

### Step 1: Enable Test Mode

Edit `src/main.c` and change:
```c
#define RUN_CRITICAL_INSTANT_TEST  0
```
to:
```c
#define RUN_CRITICAL_INSTANT_TEST  1
```

### Step 2: Rebuild

```bash
cd /home/manjeri998/SETR/SETR/freertos/build
make -j4
```

### Step 3: Flash to Pico

```bash
# Copy the .uf2 file to the Pico
cp tetris_freertos.uf2 /media/$USER/RPI-RP2/
```

Or drag-and-drop `build/tetris_freertos.uf2` to the RPI-RP2 drive.

### Step 4: Connect to Serial Output

Open PuTTY or screen to view results:
```bash
# Find the device (usually /dev/ttyACM0 or /dev/ttyACM1)
ls -l /dev/ttyACM*

# Connect with screen
screen /dev/ttyACM0 115200

# Or with PuTTY (configure Serial, 115200 baud, /dev/ttyACM0)
```

### Step 5: Observe Test Execution

You will see:
1. Test banner and setup messages
2. Tasks reporting ready at barrier
3. Simultaneous release announcement
4. Each task completing and reporting execution time
5. CSV trace dump
6. Worst-case response time analysis with deadline checks

## Expected Output Format

```
╔════════════════════════════════════════════════════════════╗
║           RUNNING IN CRITICAL INSTANT TEST MODE           ║
║   All tasks will be released simultaneously at t=0         ║
║   This tests WORST-CASE response time under maximum       ║
║   interference from all other tasks                        ║
╚════════════════════════════════════════════════════════════╝

[CRITICAL] Input task ready, waiting at barrier...
[CRITICAL] Update task ready, waiting at barrier...
[CRITICAL] Display task ready, waiting at barrier...
[COORDINATOR] All tasks ready! Preparing to release...

╔══════════════════════════════════════════════════════════╗
║  RELEASING ALL TASKS NOW - CRITICAL INSTANT at t=0!     ║
╚══════════════════════════════════════════════════════════╝

[CRITICAL] Input task RELEASED at t=0!
[CRITICAL] Update task RELEASED at t=0!
[CRITICAL] Display task RELEASED at t=0!

[CRITICAL] Input COMPLETED: exec=XXX us
[CRITICAL] Update COMPLETED: exec=XXX us
[CRITICAL] Display COMPLETED: exec=XXX us

════════════════════════════════════════════════════════════
           WORST-CASE RESPONSE TIME ANALYSIS
════════════════════════════════════════════════════════════

Task: Input
  Execution Time:  XXX µs
  Response Time:   XXX µs  ← WCRT under critical instant
  Deadline:        20000 µs
  Status:          ✓ MET (XX.X% margin)

Task: Update
  Execution Time:  XXX µs
  Response Time:   XXX µs  ← WCRT under critical instant
  Deadline:        33000 µs
  Status:          ✓ MET (XX.X% margin)

Task: Display
  Execution Time:  XXX µs
  Response Time:   XXX µs  ← WCRT under critical instant
  Deadline:        33000 µs
  Status:          ✓ MET (XX.X% margin)

════════════════════════════════════════════════════════════
```

## Interpretation

### Response Time Components

For each task under critical instant:

**Response Time = End Time - Release Time**

This includes:
- **Waiting time**: Time spent in ready queue while higher-priority tasks execute
- **Execution time**: Actual CPU time for the task's work
- **Preemption overhead**: Context switch costs (minimal in FreeRTOS)

### What to Look For

✅ **All tasks meet deadlines**: Response time < Deadline for all tasks
- Input: < 20,000 µs (20 ms)
- Update: < 33,000 µs (33.3 ms)
- Display: < 33,000 µs (33.3 ms)

✅ **Response time > Execution time**: Confirms interference from other tasks

✅ **Lower priority tasks have longer response times**: Display should have the longest response time since it's preempted by Input and Update

### Comparison with Normal Operation

Compare these WCRT values with the normal operation results from `TIMING_ANALYSIS_REPORT.md`:

| Task    | Normal Max Response | Critical Instant WCRT | Difference |
|---------|--------------------:|----------------------:|-----------:|
| Input   | ~291 µs            | ??? µs                | ??? µs     |
| Update  | ~182 µs            | ??? µs                | ??? µs     |
| Display | ~1887 µs           | ??? µs                | ??? µs     |

The critical instant WCRT should be **equal to or greater than** the normal operation maximum.

## Switching Back to Normal Mode

After completing the test:

1. Edit `src/main.c` and change:
   ```c
   #define RUN_CRITICAL_INSTANT_TEST  1
   ```
   back to:
   ```c
   #define RUN_CRITICAL_INSTANT_TEST  0
   ```

2. Rebuild:
   ```bash
   cd /home/manjeri998/SETR/SETR/freertos/build
   make -j4
   ```

3. Reflash the Pico with the updated `tetris_freertos.uf2`

## Theory Background

### Why Critical Instant Matters

In Rate-Monotonic Analysis (RMA), the **worst-case response time** occurs at the critical instant. By testing this scenario explicitly, we can:

1. **Validate schedulability**: Prove that even in the worst case, all deadlines are met
2. **Measure true WCRT**: Get empirical values for comparison with theoretical bounds
3. **Identify bottlenecks**: See which task is closest to its deadline
4. **Demonstrate robustness**: Show the system can handle maximum interference

### Priority Inversion Note

This test focuses on **rate-monotonic priority** interference. It does NOT test:
- Priority inversion from shared resources (we don't use mutexes)
- Blocking time from I/O operations (our tasks are compute-bound)
- Sporadic task interference (Audio task is not included in critical instant)

### RMA Utilization Bound

From the normal operation analysis:
- **Total Utilization**: 7.8%
- **RMA Bound (n=3)**: 75.7%
- **Margin**: 67.9 percentage points

Since U < U_bound, the system is **theoretically schedulable**. The critical instant test provides **empirical confirmation** of this theoretical result.

## Questions to Answer in Presentation

1. **What is the WCRT for each task?** (from test output)
2. **Do all tasks meet their deadlines?** (should be YES)
3. **How does WCRT compare to normal operation?** (should be higher or equal)
4. **What percentage of deadline is used?** (should be well under 100%)
5. **Why is Display's response time the longest?** (lowest priority, maximum interference)

## Troubleshooting

### Problem: Tasks don't release simultaneously

**Symptom**: Tasks report different release timestamps

**Cause**: Semaphore give/take overhead

**Solution**: This is expected and acceptable. The overhead is ~1-2 µs, which is negligible compared to task execution times (hundreds of µs to ms).

### Problem: Test runs but no trace output

**Symptom**: Test banner appears but no CSV dump

**Cause**: Trace buffer not large enough or USB output buffer overflow

**Solution**: In `project_config.h`, increase:
```c
#define TRACE_BUFFER_SIZE  1024  // Was 512
```

### Problem: Compilation errors

**Symptom**: Build fails after enabling test

**Cause**: Missing includes or undeclared functions

**Solution**: Verify all required headers are included:
- `critical_instant_test.h` in `main.c` ✓
- `task_trace.h` in `critical_instant_test.c` ✓
- All normal task headers ✓

## File Checklist

Before running test, verify these files exist:

- ✓ `src/critical_instant_test.c` (implementation)
- ✓ `include/critical_instant_test.h` (header)
- ✓ `src/main.c` (with RUN_CRITICAL_INSTANT_TEST flag)
- ✓ `CMakeLists.txt` (includes critical_instant_test.c)
- ✓ Build successful: `build/tetris_freertos.uf2`

## Next Steps After Test

1. **Capture output**: Save PuTTY log or screen session
2. **Extract WCRT values**: Note response times for each task
3. **Calculate margins**: (Deadline - WCRT) / Deadline × 100%
4. **Compare with theory**: RMA predicts schedulability; test confirms it
5. **Document in report**: Add critical instant results to timing analysis
6. **Create presentation slides**: Show comparison table and deadline margins

## References

- Liu, C. L., & Layland, J. W. (1973). "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment." *Journal of the ACM*, 20(1), 46-61.
- FreeRTOS documentation: https://www.freertos.org/
- RP2040 datasheet: https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf
