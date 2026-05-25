cmake_minimum_required(VERSION 3.13)

# ─── Pico SDK ────────────────────────────────────────────────────────────────
# PICO_SDK_PATH must be set as an environment variable or CMake cache variable.
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tetris_freertos C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMAND ON)

pico_sdk_init()

# ─── FreeRTOS Kernel ─────────────────────────────────────────────────────────
# FREERTOS_KERNEL_PATH must point to the cloned FreeRTOS-Kernel directory.
set(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH} CACHE PATH "Path to FreeRTOS Kernel")
set(FREERTOS_PORT GCC_RP2040 CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")

# FreeRTOSConfig.h lives in this project directory
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR})

include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# ─── Application ─────────────────────────────────────────────────────────────
add_executable(tetris_freertos
    main.c
    board/board_init.c
    drivers/lsm9ds0_accel.c
    state/game_state.c
    tasks/display_task.c
    tasks/input_task.c
    tasks/update_task.c
    pcd8544/pcd8544.c
)

target_include_directories(tetris_freertos PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(tetris_freertos
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_gpio
    hardware_timer
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

# Enable USB serial output (disable UART if not needed)
pico_enable_stdio_usb(tetris_freertos 1)
pico_enable_stdio_uart(tetris_freertos 0)

# Generate .uf2 file for drag-and-drop flashing
pico_add_extra_outputs(tetris_freertos)
