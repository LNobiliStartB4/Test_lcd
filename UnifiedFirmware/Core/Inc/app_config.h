#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define BENCH_TEST_MODE 0

/*
 * Temporary bench setting: set to 0 while the pneumatic setup has known leaks.
 * When enabled, Bandy waits for the control target before consuming tag time and
 * reports LEAK_TIMEOUT if the target is not reached in time.
 */
#define APPLICATION_BANDY_LEAK_GUARD_ENABLED 1

#endif
