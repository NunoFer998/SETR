# How to Choose Trace Analysis Method

You have **3 options** for viewing timing measurements. Choose based on what you need.

---

## Option 1: Raw CSV Data (Current Default) 📄

**Best for:** Detailed analysis in Excel/Python, generating graphs

**What you get:**
```
task_id,task_name,release_us,start_us,end_us,exec_us,response_us
0,Input,1234567,1234570,1234572,2,5
1,Update,1234600,1234605,1234608,3,8
... (512 entries)
```

**How to use:**
```c
// In main.c (CURRENT CONFIGURATION):
xTaskCreate(task_trace_dump, "Dump", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```

**Steps:**
1. Open PuTTY, connect to USB serial
2. Flash program, wait 10 seconds
3. Copy all CSV output from PuTTY
4. Paste into Excel for analysis

**Pros:** Full data, can generate graphs  
**Cons:** Raw data needs post-processing

---

## Option 2: Clean Summary Statistics 📊 **RECOMMENDED**

**Best for:** Quick verification, presentations, reports

**What you get:**
```
╔════════════════════════════════════════════════╗
║      TIMING ANALYSIS RESULTS (10 sec)         ║
╚════════════════════════════════════════════════╝

Task: Input    (500 jobs recorded)
├─ Execution Time:
│  ├─ Min:     180 µs
│  ├─ Avg:     195 µs
│  └─ Max:     220 µs  ← WCET
│
├─ Response Time:
│  ├─ Min:     180 µs
│  ├─ Avg:     350 µs
│  └─ Max:     450 µs  ← WCRT
│
└─ Deadline: 20000 µs (20 ms) — ✓ MET

... (for all 4 tasks)

FREERTOS CPU USAGE:
Input     4%
Update    2%
Display   3%
Audio     1%
IDLE     90%
```

**How to use:**
```c
// In main.c, REPLACE the dump task line with:
xTaskCreate(task_analysis_summary, "Analysis", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```

**Steps:**
1. Edit `main.c` (see code above)
2. Rebuild and flash
3. View clean output in PuTTY after 10 seconds

**Pros:** Easy to read, no post-processing needed  
**Cons:** No raw data for detailed analysis

---

## Option 3: Both Statistics + CSV 📊📄

**Best for:** When you want quick summary AND detailed data

**What you get:**
- Summary statistics (like Option 2)
- Full CSV data (like Option 1)
- FreeRTOS CPU usage

**How to use:**
```c
// In main.c, REPLACE the dump task line with:
xTaskCreate(task_analysis_full, "Analysis", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```

**Pros:** Best of both worlds  
**Cons:** Lots of output to scroll through

---

## Step-by-Step: How to Switch

### Currently you have (Option 1 - CSV):
```c
// In src/main.c line ~47:
xTaskCreate(task_trace_dump, "Dump", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```

### To switch to Option 2 (Clean Summary):
1. Open `/home/manjeri998/SETR/SETR/freertos/src/main.c`
2. Find line ~47 (the dump task creation)
3. Change it to:
```c
xTaskCreate(task_analysis_summary, "Analysis", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```
4. Update CMakeLists.txt to add analysis_task.c (see below)
5. Rebuild: `cd build && make`

### To switch to Option 3 (Both):
Same as above but use:
```c
xTaskCreate(task_analysis_full, "Analysis", STACK_DUMP, NULL, PRIORITY_DUMP, NULL);
```

---

## CMakeLists.txt Update (For Options 2 or 3)

You need to add the new analysis_task.c to the build:

```cmake
# In freertos/CMakeLists.txt, find the add_executable section and add:
add_executable(tetris_freertos
    src/main.c
    src/board_init.c
    src/display_task.c
    src/input_task.c
    src/update_task.c
    src/game_state.c
    src/tetris_logic.c
    src/pcd8544.c
    src/lsm9ds0_accel.c
    src/audio_task.c
    src/task_trace.c
    src/analysis_task.c    # ← ADD THIS LINE
)
```

---

## My Recommendation

**For your project presentation:** Use **Option 2** (Clean Summary)

Why?
- Shows WCET and WCRT clearly
- Deadline verification obvious
- Professional looking output
- No need to process data externally
- Still shows CPU utilization

**If your instructor wants raw data:** Keep **Option 1** (CSV) or use **Option 3** (Both)

---

## Quick Comparison Table

| Feature | Option 1 (CSV) | Option 2 (Summary) | Option 3 (Both) |
|---------|---------------|-------------------|----------------|
| WCET/WCRT | ✓ (calculate) | ✓ (shown) | ✓ (shown) |
| Deadline check | Manual | Automatic ✓ | Automatic ✓ |
| CPU usage | ✓ | ✓ | ✓ |
| All 512 entries | ✓ | ✗ | ✓ |
| Excel export | ✓ | ✗ | ✓ |
| Easy to read | ✗ | ✓ | Mixed |
| Setup effort | None (current) | Edit 2 files | Edit 2 files |

---

## Current Configuration

You're currently using **Option 1 (CSV)** - no changes needed if that works for you!

To try Option 2, follow the steps above to:
1. Edit main.c (change function name)
2. Update CMakeLists.txt (add analysis_task.c)
3. Rebuild and flash
