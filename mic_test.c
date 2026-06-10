#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define MIC_PIN 15

volatile uint32_t irq_count = 0;
volatile uint32_t rise_count = 0;
volatile uint32_t fall_count = 0;

void gpio_callback(uint gpio, uint32_t events) {
    irq_count++;
    if (events & GPIO_IRQ_EDGE_RISE) {
        rise_count++;
    }
    if (events & GPIO_IRQ_EDGE_FALL) {
        fall_count++;
    }
}

int main() {
    stdio_init_all();
    
    printf("\n\n=== Microphone Pin Test ===\n");
    printf("Testing GP%d\n\n", MIC_PIN);
    
    // Initialize GPIO
    gpio_init(MIC_PIN);
    gpio_set_dir(MIC_PIN, GPIO_IN);
    
    printf("Test 1: No pull resistor\n");
    gpio_disable_pulls(MIC_PIN);
    sleep_ms(100);
    printf("  GP%d state: %d\n\n", MIC_PIN, gpio_get(MIC_PIN));
    
    printf("Test 2: Pull-down enabled\n");
    gpio_pull_down(MIC_PIN);
    sleep_ms(100);
    printf("  GP%d state: %d\n\n", MIC_PIN, gpio_get(MIC_PIN));
    
    printf("Test 3: Pull-up enabled\n");
    gpio_pull_up(MIC_PIN);
    sleep_ms(100);
    printf("  GP%d state: %d\n\n", MIC_PIN, gpio_get(MIC_PIN));
    
    printf("Test 4: Monitoring both edges (make noise for 10 seconds)...\n");
    
    // Enable interrupts on BOTH edges
    gpio_set_irq_enabled_with_callback(MIC_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    
    uint32_t last_poll_state = gpio_get(MIC_PIN);
    uint32_t poll_changes = 0;
    
    for (int i = 0; i < 100; i++) {
        sleep_ms(100);
        
        uint32_t current_state = gpio_get(MIC_PIN);
        if (current_state != last_poll_state) {
            poll_changes++;
            printf("  [POLL] GP%d: %d -> %d (change #%lu)\n", 
                   MIC_PIN, last_poll_state, current_state, (unsigned long)poll_changes);
            last_poll_state = current_state;
        }
        
        if (i % 10 == 9) {
            printf("  After %d sec: IRQ=%lu RISE=%lu FALL=%lu POLL_CHANGES=%lu GP%d=%d\n",
                   i/10 + 1,
                   (unsigned long)irq_count,
                   (unsigned long)rise_count,
                   (unsigned long)fall_count,
                   (unsigned long)poll_changes,
                   MIC_PIN,
                   gpio_get(MIC_PIN));
        }
    }
    
    printf("\n=== Final Results ===\n");
    printf("Total IRQ events: %lu\n", (unsigned long)irq_count);
    printf("  Rising edges:   %lu\n", (unsigned long)rise_count);
    printf("  Falling edges:  %lu\n", (unsigned long)fall_count);
    printf("Polling detected: %lu changes\n", (unsigned long)poll_changes);
    
    if (poll_changes > 0 && irq_count == 0) {
        printf("\n** PROBLEM: Pin changes detected by polling but NOT by interrupt! **\n");
        printf("   This suggests an interrupt configuration issue.\n");
    } else if (poll_changes == 0) {
        printf("\n** PROBLEM: No pin changes detected at all! **\n");
        printf("   Check hardware connections:\n");
        printf("   1. Is OUT pin connected to GP15?\n");
        printf("   2. Is VCC connected to 3.3V or 5V?\n");
        printf("   3. Is GND connected to ground?\n");
        printf("   4. Does the LED on the mic module light up when you make noise?\n");
    } else {
        printf("\nHardware and interrupts working correctly!\n");
    }
    
    while (true) {
        tight_loop_contents();
    }
}
