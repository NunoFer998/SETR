#ifndef CRITICAL_INSTANT_TEST_H
#define CRITICAL_INSTANT_TEST_H

/**
 * @brief Run critical instant test
 * 
 * Tests worst-case response time by releasing all tasks simultaneously.
 * This represents the "critical instant" in real-time scheduling theory.
 * 
 * Call this from main() instead of normal task creation to run the test.
 */
void run_critical_instant_test(void);

#endif /* CRITICAL_INSTANT_TEST_H */
