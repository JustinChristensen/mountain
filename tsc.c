#include <stdio.h>
#include <stdlib.h>

int main() {
    unsigned eax = 0x15; // time stamp counter and nominal core crystal clock information leaf
    unsigned den, num, core_freq_hz, res;

    asm volatile (
        "cpuid\n\t" :
        "=a" (den), "=b" (num), "=c" (core_freq_hz), "=d" (res) :
        "a" (eax)
    );

    printf("den: %u, num: %u, core_freq_hz: %u, res: %u\n", den, num, core_freq_hz, res);
}
