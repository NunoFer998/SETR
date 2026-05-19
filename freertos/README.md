# FreeRTOS Tetris — Preemptive Scheduler for Raspberry Pi Pico

Ported from the MicroPython cooperative scheduler to C + FreeRTOS with:
- **Preemptive scheduling** — higher-priority tasks interrupt lower-priority ones
- **Priority levels** — Input (3) > Update (2) > Display (1)
- **Mutexes with priority inheritance** — no unbounded priority inversion

## Prerequisites

1. **Pico SDK** — [Installation guide](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
2. **FreeRTOS Kernel** — Clone into this directory:
   ```bash
   cd freertos
   git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git
   cd FreeRTOS-Kernel
   git submodule update --init
   ```
3. **CMake** (≥ 3.13) and **ARM GCC toolchain**

## Environment Variables

Set these before building:

```bash
# Point to your Pico SDK installation
export PICO_SDK_PATH=/path/to/pico-sdk

# Point to the FreeRTOS kernel (cloned above)
export FREERTOS_KERNEL_PATH=/path/to/freertos/FreeRTOS-Kernel
```

## Build

```bash
cd freertos
mkdir build && cd build
cmake ..
make -j4
```

This produces `tetris_freertos.uf2` in the build directory.

## Flash

1. Hold the **BOOTSEL** button on your Pico
2. Connect via USB (it appears as a mass storage device)
3. Drag and drop `tetris_freertos.uf2` onto the drive

## Pin Wiring

| Function     | GPIO | Peripheral |
|-------------|------|------------|
| I2C SDA     | GP0  | LSM9DS0    |
| I2C SCL     | GP1  | LSM9DS0    |
| SPI SCK     | GP6  | PCD8544    |
| SPI MOSI    | GP7  | PCD8544    |
| LCD CS      | GP5  | PCD8544    |
| LCD DC      | GP4  | PCD8544    |
| LCD RST     | GP8  | PCD8544    |
| LCD BL      | GP9  | PCD8544    |

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                 FreeRTOS Kernel                      │
│         (preemptive, priority-based)                │
├──────────┬──────────────┬──────────────────────────┤
│ Input    │ Update       │ Display                   │
│ Prio: 3  │ Prio: 2      │ Prio: 1                  │
│ 30ms     │ 30ms         │ 30ms                      │
│          │              │                            │
│ Reads    │ Reads        │ Reads square_x            │
│ accel    │ accel_x      │ under display_mutex       │
│ via I2C  │ under        │                            │
│          │ accel_mutex  │ Renders to LCD             │
│ Writes   │              │ via SPI                    │
│ accel_x  │ Writes       │                            │
│ under    │ square_x     │                            │
│ accel_   │ under        │                            │
│ mutex    │ display_     │                            │
│          │ mutex        │                            │
└──────────┴──────────────┴──────────────────────────┘
```

## What Changed from MicroPython

| Feature | MicroPython (before) | C + FreeRTOS (now) |
|---------|---------------------|--------------------|
| Scheduling | Cooperative (poll loop) | Preemptive (SysTick/PendSV) |
| Priorities | None (insertion order) | 3 levels with preemption |
| Shared state | Unprotected globals | Mutex with priority inheritance |
| Timing | Timer ISR + manual dispatch | `vTaskDelayUntil()` — precise |
| Context switch | Not possible | Hardware register save/restore |
