#include <stdio.h>
#include <stdlib.h>
#include <cpuid.h>
#include <inttypes.h>

enum cpuid_leafs {
    TSC_LEAF = 0x15,
    PROC_FREQ_LEAF = 0x16,
    ARCH_PERFMON_LEAF = 0x0a
};

enum msrs {
    PERF_EVTSEL = 0x186,
    PERF_GLOBAL_STATUS = 0x38e,
    PERF_GLOBAL_CTRL = 0x38f
};

static uint64_t rdmsr(unsigned msr) {
    uint64_t hi;
    uint64_t lo;
    asm volatile ("rdmsr": "=d" (hi), "=a" (lo) : "c" (msr));
    return hi << 32 | lo;
}

static char *eflags(unsigned flags, unsigned bit) {
    return (flags >> bit) & 1 ? "not available" : "available";
}

int main() {
    unsigned den, num, core_freq_hz, res;
    __get_cpuid(TSC_LEAF, &den, &num, &core_freq_hz, &res);
    printf("den: %u\nnum: %u\ncore_freq_hz: %u\n",
            den, num, core_freq_hz);

    unsigned proc_base_freq_mhz, max_freq_mhz, bus_freq_mhz;
    __get_cpuid(PROC_FREQ_LEAF, &proc_base_freq_mhz, &max_freq_mhz, &bus_freq_mhz, &res);
    printf("proc_base_freq_mhz: %u\nmax_freq_mhz: %u\nbus_freq_mhz: %u\n",
            proc_base_freq_mhz, max_freq_mhz, bus_freq_mhz);

    // this information is available via sysctl on OSX:
    // sysctl machdep.cpu.arch_perf
    unsigned perf, perf_flags, perf_ffunc;
    __get_cpuid(ARCH_PERFMON_LEAF, &perf, &perf_flags, &res, &perf_ffunc);
    printf("perfmon_version_id: %u\n"
           "num_regs_per_logical_proc: %u\n"
           "regs_bit_width: %u\n"
           "ebx_bit_vector_length: %u\n",
           perf & 0xFF, (perf >> 8) & 0xFF, (perf >> 16) & 0xFF, (perf >> 24) & 0xFF);

    printf("core_cycle_event: %s\n"
           "instruction_retired_event: %s\n"
           "reference_cycles_event: %s\n"
           "last_level_cache_reference_event: %s\n"
           "last_level_cache_misses_event: %s\n"
           "branch_instruction_retired_event: %s\n"
           "branch_mispredict_retired_event: %s\n",
           eflags(perf_flags, 0),
           eflags(perf_flags, 1),
           eflags(perf_flags, 2),
           eflags(perf_flags, 3),
           eflags(perf_flags, 4),
           eflags(perf_flags, 5),
           eflags(perf_flags, 6));

    printf("fixed_func_perf_counters: %u\n"
           "fixed_func_perf_counters_bit_width: %u\n",
           perf_ffunc & 0xF, (perf_ffunc >> 4) & 0xFF);

    // uint64_t status = rdmsr(PERF_GLOBAL_STATUS),
    //          esel = rdmsr(PERF_EVTSEL),
    //          ctrl = rdmsr(PERF_GLOBAL_CTRL);

    // printf("status: %"PRIu64"\n"
    //        "esel: %"PRIu64"\n"
    //        "ctrl: %"PRIu64"\n",
    //         status, esel, ctrl);
}
