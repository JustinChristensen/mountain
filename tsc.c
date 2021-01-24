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

    printf("den: %u\nnum: %u\ncore_freq_hz: %u\nres: %u\n", den, num, core_freq_hz, res);

    eax = 0x16; // time stamp counter and nominal core crystal clock information leaf
    unsigned proc_base_freq_mhz, max_freq_mhz, bus_freq_mhz;

    asm volatile (
        "cpuid\n\t" :
        "=a" (proc_base_freq_mhz), "=b" (max_freq_mhz), "=c" (bus_freq_mhz), "=d" (res) :
        "a" (eax)
    );

    printf("proc_base_freq_mhz: %u\nmax_freq_mhz: %u\nbus_freq_mhz: %u\nres: %u\n", proc_base_freq_mhz, max_freq_mhz, bus_freq_mhz, res);
}
