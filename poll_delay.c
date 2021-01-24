#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

static uint64_t rdtsc() {
    uint64_t tsc; uint32_t lo;
    asm volatile ("lfence\n\trdtsc\n\tlfence" : "=a" (lo), "=d" (tsc));
    // asm volatile ("rdtsc" : "=a" (lo), "=d" (tsc));
    return (tsc << 32) | lo;
}

static void pause() {
    asm volatile ("pause");
}

static bool before(uint64_t a, uint64_t b) {
    return a < b;
}

static void poll_delay(uint32_t cycles) {
    uint64_t end = rdtsc() + cycles;
    while (before(rdtsc(), end)) pause();
}

int main() {
    for (int i = 0; i < 300; i++) {
        poll_delay(2900000000);
        printf("\033c%d\n", i);
    }
}
