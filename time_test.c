#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <mach/mach_time.h>

#define LOOP_END ((1 << 30) >> 1)
#define ONE_SEC_NS 1000000000

int __attribute__((noinline)) clock_sleep_test() {
    // for example
    // CLOCKS_PER_SEC = 1000000
    // clock = 810170 microseconds
    // seconds = clock / CLOCKS_PER_SEC
    // 0.810170 seconds
    clock_t cl = clock();
    struct timespec ts = { 1, 0 };
    if (nanosleep(&ts, NULL)) {
        perror(NULL);
        return 1;
    }

    printf("sleep: %lu microseconds\n", clock() - cl);

    return 0;
}

int __attribute__((noinline)) clock_loop_test() {
    clock_t cl = clock();
    for (volatile long i = 0; i < LOOP_END; i++);
    printf("clock loop: %lu microseconds\n", clock() - cl);
    return 0;
}

int __attribute__((noinline)) gettime_loop_test() {
    struct timespec start;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &start)) {
        perror(NULL);
        return 1;
    }

    for (volatile long i = 0; i < LOOP_END; i++);

    struct timespec end;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &end)) {
        perror(NULL);
        return 1;
    }

    if (start.tv_nsec > end.tv_nsec) {
        end.tv_sec--;
        end.tv_nsec += ONE_SEC_NS;
    }

    end.tv_sec -= start.tv_sec;
    end.tv_nsec -= start.tv_nsec;

    printf("gettime loop: %lu nanoseconds\n", ONE_SEC_NS * end.tv_sec + end.tv_nsec);

    return 0;
}

unsigned long rdtsc() {
    unsigned long tsc; unsigned lo;
    asm volatile ("lfence\n\trdtsc\n\tlfence" : "=a" (lo), "=d" (tsc));
    return (tsc << 32) | lo;
}

int __attribute__((noinline)) tsc_loop_test() {
    unsigned long start = rdtsc();
    for (volatile long i = 0; i < LOOP_END; i++);
    printf("rdtsc loop: %lu cycles\n", rdtsc() - start);
    return 0;
}

int __attribute__((noinline)) mach_time_loop_test() {
    uint64_t start = mach_absolute_time();
    for (volatile long i = 0; i < LOOP_END; i++);
    printf("mach_absolute_time loop: %"PRIu64" nanoseconds\n", mach_absolute_time() - start);
    return 0;
}

int __attribute__((noinline)) mhz_test() {
    struct timespec ts = { 2, 0 };
    unsigned long start = rdtsc();
    nanosleep(&ts, NULL);
    printf("clock speed: %.1lf MHz\n", (rdtsc() - start) / (2 * 1e6));
    return 0;
}

int main() {
    return
        clock_sleep_test() ||
        clock_loop_test() ||
        gettime_loop_test() ||
        tsc_loop_test() ||
        mach_time_loop_test() ||
        mhz_test();
}
