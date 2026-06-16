# Audio Sporadic Server Design

## Overview

The audio input system uses a **Sporadic Server** to handle aperiodic microphone interrupts while guaranteeing bounded interference to other real-time tasks.

## Game Mechanic

**Blow into the microphone to make the Tetris piece fall faster!**

- **Normal behavior:** Piece falls at regular gravity speed
- **While blowing:** Piece accelerates (soft drop activated continuously)
- **Budget exhausted:** Piece returns to normal speed despite continued blowing
- **After 1 second:** Budget replenishes, acceleration resumes

## Why Sporadic Server?

### The Problem

Audio interrupts are **aperiodic** (arrive unpredictably):
- User can blow continuously at ~50-100 Hz
- Without control, audio task could monopolize CPU
- Higher priority audio task would starve lower priority tasks
- Update and display tasks would miss deadlines
- Screen would freeze, game becomes unresponsive

### The Solution

Sporadic Server converts aperiodic audio events into **bounded periodic behavior**:

```
Parameters:
- Execution budget (C): 5000 μs (5 ms) per period
- Replenishment period (T): 1000 ms (1 second)
- Priority: 4 (highest)
```

**Guarantee:** Audio task consumes at most 5ms of CPU time per second, regardless of interrupt rate.

## How It Works

### 1. Interrupt Arrives
```c
GPIO falling edge detected → ISR sets audio_request_pending
```

### 2. Task Wakes and Checks Budget
```c
if (execution_budget_us == 0) {
    // No budget available - defer event
    continue;
}
```

### 3. Process Event with Time Measurement
```c
uint32_t start = time_us_32();
g_tetris_state.poll.soft_drop_activated = true;  // Accelerate piece
uint32_t consumed = time_us_32() - start;
execution_budget_us -= consumed;
```

### 4. Schedule Replenishment
```c
// Restore consumed budget 1 second from now
schedule_replenishment(consumed, current_time + 1000ms);
```

## Demonstrating Sporadic Server Benefits

### Scenario 1: Normal Use
```
User blows: ───█──────█──────█─────
Piece falls: ▼──▼▼──▼▼▼──▼▼──▼───
Budget:      ████████████████████
Result: ✓ Smooth acceleration, system responsive
```

### Scenario 2: Continuous Blowing (Budget Exhausted)
```
User blows: █████████████████████
            ↓
Budget:     ████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
            ↑ exhausted after 5ms
Piece falls: ▼▼▼▼▼────────────────
            (stops accelerating)
Game tasks: ✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓
            (continue normally)

After 1 sec:
Budget:     ████▒▒▒▒▒▒▒▒▒████▒▒▒▒
                        ↑ replenished
Piece falls: ▼▼▼▼▼─────────▼▼▼▼▼──
Result: ✓ Graceful degradation, no freeze
```

### Without Sporadic Server
```
User blows: █████████████████████
Audio task: ███████████████████ (monopolizes CPU)
Update:     ──────────────────── (starved)
Display:    ──────────────────── (starved)
Result: ✗ Screen freezes, game unplayable
```

## Schedulability Analysis

### Task Set
| Task    | Period (T) | WCET (C) | Priority | Utilization |
|---------|------------|----------|----------|-------------|
| Audio   | 1000 ms    | 5 ms     | 4        | 0.005       |
| Input   | 20 ms      | 2 ms     | 3        | 0.100       |
| Update  | 33 ms      | 3 ms     | 2        | 0.091       |
| Display | 33 ms      | 4 ms     | 1        | 0.121       |
| **Total** |          |          |          | **0.317**   |

**Rate Monotonic Bound:** U ≤ 4(2^(1/4) - 1) ≈ 0.757

**Result:** 0.317 < 0.757 → **System is schedulable!**

## Implementation Details

### Event Consolidation
Multiple interrupts during processing are consolidated into one event to prevent rapid-fire activations.

### Minimal Debouncing
20ms debounce filters hardware bouncing while allowing continuous input (50 Hz max).

### Budget Tracking
Actual CPU time measured using hardware timer (`time_us_32()`), not estimated.

### Replenishment Queue
Circular buffer tracks multiple pending replenishments, allowing burst handling.

## Configuration

```c
// In project_config.h
#define AUDIO_SERVER_BUDGET_US    5000   // 5ms CPU time per period
#define AUDIO_SERVER_PERIOD_MS    1000   // 1 second replenishment

// In audio_task.c
#define AUDIO_DEBOUNCE_MS  20            // Minimal bounce filtering
```

## Testing and Validation

### Test 1: Normal Responsiveness
**Action:** Blow once briefly  
**Expected:** Piece accelerates, then returns to normal  
**Validates:** Basic functionality

### Test 2: Continuous Load
**Action:** Blow continuously for 5+ seconds  
**Expected:** 
- First 5ms: Piece accelerates rapidly
- After budget exhausted: Piece falls at normal speed
- Game remains responsive (screen updates, input works)
- After 1 sec: Acceleration resumes briefly  
**Validates:** Budget enforcement, graceful degradation

### Test 3: Timing Measurements
**Action:** Log execution times and budget consumption  
**Expected:**
- Each event consumes 20-100μs
- Budget never goes negative
- Replenishments occur at expected times  
**Validates:** Correct SS algorithm implementation

## Benefits Demonstrated

1. ✅ **Bounded Interference**: Audio can't starve other tasks
2. ✅ **Predictable Timing**: Converts aperiodic to periodic for analysis
3. ✅ **Graceful Degradation**: System stays responsive under overload
4. ✅ **Provable Schedulability**: Can formally prove deadline guarantees
5. ✅ **Real-Time Guarantees**: Update/display tasks meet their deadlines

## References

- Liu, C.L., & Layland, J.W. (1973). "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
- Sprunt, B., Sha, L., & Lehoczky, J. (1989). "Aperiodic Task Scheduling for Hard-Real-Time Systems"
- Buttazzo, G. (2011). "Hard Real-Time Computing Systems: Predictable Scheduling Algorithms and Applications"
