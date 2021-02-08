#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>
#include <cpuid.h>

#define MAX_FLAG 128

enum arg_type {
    UNKNOWN_ARG,
    MISSING_VAL,
    INVALID_VAL,
    FLAG_OVERFLOW,
    HELP,
    VERSION,
    STRIDE_INTERVAL,
    START_STRIDE,
    END_STRIDE,
    MIN_SIZE,
    MAX_SIZE,
    SHIFT_SAMPLES,
    PRIME_CACHE,
    USE_RDTSC,
    BENCHMARK
};

enum benchmark {
    UINT64,
    UINT64_SINK,
    AVX2,
    AVX2_SINK
};

struct arg {
    enum arg_type type;
    char flag[MAX_FLAG];
    union {
        uint8_t u8;
        char *s;
        enum benchmark b;
    };
};

struct args {
    uint8_t stride_interval, // interval=[1,n] where interval divides max_stride
            start_stride,    // stride=[1,n] where start < end
            end_stride,
            min_size_p2,     // size=2^n where n=[10,27] and min_size < max_size
            max_size_p2,
            shift_samples;
    bool prime_cache, use_rdtsc;
    enum benchmark benchmark;
};

struct bench_params {
    bool prime_cache, use_rdtsc;
    uint8_t k;
    unsigned
        max_samples,
        shift_samples,
        denom,
        base_spread;
};

static bool debug(char const *key) {
    char *dbg = getenv("DEBUG");
    return dbg != NULL && ((key && !strcmp(key, dbg)) || !strcmp("*", dbg));
}

static void uint8_val(char const *s, struct arg *arg) {
    char *end = NULL;
    long n = strtol(s, &end, 10);

    if (s == end) {
        arg->type = INVALID_VAL;
        fprintf(stderr, "parse error on value %s of flag %s\n", s, arg->flag);
        return;
    }

    if (n < 0 || n > UINT8_MAX) {
        arg->type = INVALID_VAL;
        fprintf(stderr, "value %s for flag %s out of expected range: [0,255]\n", s, arg->flag);
        return;
    }

    arg->u8 = (uint8_t) n;
}

static void benchmark_val(char const *s, struct arg *arg) {
    if (!strcmp(s, "uint64"))           arg->b = UINT64;
    else if (!strcmp(s, "uint64_sink")) arg->b = UINT64_SINK;
    else if (!strcmp(s, "avx2"))        arg->b = AVX2;
    else if (!strcmp(s, "avx2_sink"))   arg->b = AVX2_SINK;
    else {
        arg->type = INVALID_VAL;
        fprintf(stderr, "%s is not a known benchmark\n", s);
    }
}

static void setflag(char *flag, char const *s, char const *e) {
    size_t const n = e - s < MAX_FLAG ? e - s : MAX_FLAG - 1;
    strncpy(flag, s, n);
    flag[n] = '\0';
}

static bool _parse_arg(
    char const *lflag, char sflag, enum arg_type type, void (*val)(char const *s, struct arg *arg),
    struct arg *arg, char const ***argvp
) {
    char const **argv = *argvp;
    char const *s = *argv, *a = s;

    if (*s++ != '-') // TODO: positionals
        return strcpy(arg->flag, *argv), false;

    argv++;

    char const *v = NULL;
    if (lflag && *s == '-') {
        char const *f = ++s;
        s += strcspn(s, "=");

        if (s - a < MAX_FLAG) {
            if (!*f || strncmp(lflag, f, s - f)) return false;
            setflag(arg->flag, a, s);
            arg->type = type;
        } else {
            setflag(arg->flag, a, s);
            arg->type = FLAG_OVERFLOW;
        }
    } else if (sflag && *s) {
        if (*s++ != sflag) return false;

        arg->type = type;
        setflag(arg->flag, a, s);
    } else return false;

    *argvp = argv;

    if (!val) return true;

    if (*s == '=') v = ++s;
    else if (*s)   v = s;
    else           v = *argv, *argv && argv++;

    if (!v || !*v) arg->type = MISSING_VAL;
    else           (*val)(v, arg);

    *argvp = argv;

    return true;
}

static void print_version(FILE *handle, char const *version) {
    fprintf(handle, "%s\n", version);
}

static void print_usage(FILE *handle, char const *prog) {
    char const *optfmt = "  %-32s  %s\n";
    fprintf(handle, "usage: %s [options]\n\n", prog);
    fprintf(handle, "Generate a memory mountain\n\n");
    fprintf(handle, "options:\n");
    fprintf(handle, optfmt, "-b", "Benchmark: uint64 (default), uint64_sink, avx2, avx2_sink");
    fprintf(handle, optfmt, "-n, --stride-interval", "Interval to increase the stride by (+= 2).");
    fprintf(handle, optfmt, "-s, --start-stride", "Starting stride (1).");
    fprintf(handle, optfmt, "-e, --end-stride", "Ending stride (32).");
    fprintf(handle, optfmt, "-i, --min-size", "Minimum size as a power of two (2^n where n = 10 or 1 KB).");
    fprintf(handle, optfmt, "-a, --max-size", "Maximum size as a power of two (2^n where n = 27 or 128 MB).");
    fprintf(handle, optfmt, "--shift-samples", "Shift off the minimum sample after N samples (50).");
    fprintf(handle, optfmt, "--prime-cache", "Attempt to prime the cache before entering the test loop.");
    fprintf(handle, optfmt, "-t", "Use rdtsc for tracking time.");
    fprintf(handle, "\n");
}

static void unknown_arg(struct arg *arg, char const *s) {
    char const *b = s;

    arg->type = UNKNOWN_ARG;

    if (*s == '-') {
        s++;
        if (*s == '-') s++, s += strcspn(s, "=");
        else if (*s) s++;
    } else while (*s) s++;

    setflag(arg->flag, b, s);
}

static char const **parse_arg(struct arg *arg, char const **argv) {
    bool found_arg =
       _parse_arg("help", 'h', HELP, NULL, arg, &argv)
    || _parse_arg("version", 'v', VERSION, NULL, arg, &argv)
    || _parse_arg(NULL, 'b', BENCHMARK, benchmark_val, arg, &argv)
    || _parse_arg("stride-interval", 'n', STRIDE_INTERVAL, uint8_val, arg, &argv)
    || _parse_arg("start-stride", 's', START_STRIDE, uint8_val, arg, &argv)
    || _parse_arg("end-stride", 'e', END_STRIDE, uint8_val, arg, &argv)
    || _parse_arg("min-size", 'i', MIN_SIZE, uint8_val, arg, &argv)
    || _parse_arg("max-size", 'a', MAX_SIZE, uint8_val, arg, &argv)
    || _parse_arg("shift-samples", 0, SHIFT_SAMPLES, uint8_val, arg, &argv)
    || _parse_arg("prime-cache", 0, PRIME_CACHE, NULL, arg, &argv)
    || _parse_arg(NULL, 't', USE_RDTSC, NULL, arg, &argv)
    ;

    // copy flag
    if (!found_arg) {
        unknown_arg(arg, *argv);
        argv++;
    }

    return argv;
}

static bool parse_args(struct args *args, char const *version, char const **argv) {
    char const *prog = *argv++;

    if (!*argv) return true;

    struct arg arg = { 0 };
    bool success = true;

    do {
        argv = parse_arg(&arg, argv);

        switch (arg.type) {
            case BENCHMARK:         args->benchmark = arg.b; break;
            case STRIDE_INTERVAL:   args->stride_interval = arg.u8; break;
            case START_STRIDE:      args->start_stride = arg.u8; break;
            case END_STRIDE:        args->end_stride = arg.u8; break;
            case MIN_SIZE:          args->min_size_p2 = arg.u8; break;
            case MAX_SIZE:          args->max_size_p2 = arg.u8; break;
            case SHIFT_SAMPLES:     args->shift_samples = arg.u8; break;
            case PRIME_CACHE:       args->prime_cache = true; break;
            case USE_RDTSC:         args->use_rdtsc = true; break;
            case VERSION:
                print_version(stdout, version);
                exit(0);
                break;
            case HELP:
                print_usage(stdout, prog);
                exit(0);
                break;
            case FLAG_OVERFLOW:
                fprintf(stderr, "flag overflow %s\n", arg.flag);
                success = false;
                break;
            case MISSING_VAL:
                fprintf(stderr, "missing value for flag %s\n", arg.flag);
                success = false;
                break;
            case UNKNOWN_ARG:
                fprintf(stderr, "unknown argument %s\n", arg.flag);
                success = false;
                break;
            case INVALID_VAL:
            default:
                success = false;
                break;
        }
    } while (*argv);

    if (!success)
        print_usage(stderr, prog);

    return success;
}

static void debug_args(struct args const *args) {
    fprintf(stderr, "args:\n");
    fprintf(stderr,
        "  stride_interval = %hhu\n"
        "  start_stride = %hhu\n"
        "  end_stride = %hhu\n"
        "  min_size_p2 = %hhu\n"
        "  max_size_p2 = %hhu\n"
        "  shift_samples = %hhu\n"
        "  prime_cache = %s\n"
        "  use_rdtsc = %s\n",
        args->stride_interval,
        args->start_stride,
        args->end_stride,
        args->min_size_p2,
        args->max_size_p2,
        args->shift_samples,
        args->prime_cache ? "true" : "false",
        args->use_rdtsc ? "true" : "false");
}

#define MAX_POWER 32
#define SIZE_FMT "%s size cannot be greater than %ld bytes, choose a power smaller than %d.\n"
static bool validate_args(struct args const *args) {
    bool success = true;
    if (args->min_size_p2 > MAX_POWER)
        success = false, fprintf(stderr, SIZE_FMT, "minimum", 1L << MAX_POWER, MAX_POWER);
    if (args->max_size_p2 > MAX_POWER)
        success = false, fprintf(stderr, SIZE_FMT, "maximum", 1L << MAX_POWER, MAX_POWER);
    if (args->start_stride > args->end_stride)
        success = false, fprintf(stderr, "start stride must be less than or equal to ending stride\n");
    if (args->min_size_p2 > args->max_size_p2)
        success = false, fprintf(stderr, "max size must be greater than or equal to min size\n");

    return success;
}

static uint64_t rdtsc() {
    uint64_t tsc, lo;
    asm volatile ("lfence\n\trdtsc\n\tlfence" : "=a" (lo), "=d" (tsc));
    return (tsc << 32) | lo;
}

#define ONE_SEC_NS 1000000000
static uint64_t now(bool use_rdtsc) {
    if (use_rdtsc) return rdtsc();

    struct timespec ts;

    // CLOCK_MONOTONIC on OSX only supports microsecond precision, and each nanosecond counts
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts)) {
        fprintf(stderr, "error reading current time\n");
        abort();
    }

    return ts.tv_sec * ONE_SEC_NS + ts.tv_nsec;
}

static void debug_bench_params(struct bench_params const *p) {
    fprintf(stderr, "bench_params:\n");
    fprintf(stderr,
        "  prime_cache = %s\n"
        "  k = %u\n"
        "  max_samples = %u\n"
        "  shift_samples = %u\n"
        "  denom = %u\n"
        "  base_spread = %u\n"
        "  use_rdtsc = %s\n",
        p->prime_cache ? "true" : "false",
        p->k,
        p->max_samples,
        p->shift_samples,
        p->denom,
        p->base_spread,
        p->use_rdtsc ? "true" : "false");
}

static bool has_converged(uint64_t *samples, unsigned s, struct bench_params const p) {
    uint64_t delta = samples[p.k - 1] - samples[0];
    uint64_t spread = samples[0] / p.denom + p.base_spread;

    if (s >= p.k && debug("converging"))
        fprintf(stderr, "converging... delta: %"PRIu64", spread: %"PRIu64"\n", delta, spread);

    return s >= p.k && delta < spread;
}

static void sort_samples(uint64_t *samples, unsigned i) {
    unsigned const k = i;

    while (i && samples[i - 1] > samples[i]) {
        uint64_t t = samples[i - 1];
        samples[i - 1] = samples[i];
        samples[i] = t;
        i--;
    }

    i = 0;
    if (debug("sorted")) {
        fprintf(stderr, "sorted samples (k = %u): [", k);
        fprintf(stderr, "%"PRIu64"", samples[i++]);
        while (i <= k)
            fprintf(stderr, ", %"PRIu64"", samples[i++]);
        fprintf(stderr, "]\n");
    }
}

static unsigned add_sample(
    uint64_t *samples, unsigned s, uint64_t elapsed,
    struct bench_params const p
) {
    unsigned i = s < p.k ? s : p.k - 1U;

    if (s < p.k || elapsed < samples[i])
        samples[i] = elapsed;

    return i;
}

static void try_shift(uint64_t *samples, unsigned s, struct bench_params const p) {
    if (s < p.k || s % p.shift_samples > 0) return;

    unsigned i = 0;
    while (i < p.k - 1U) samples[i] = samples[i + 1], i++;
    samples[p.k - 1] = UINT64_MAX;  // add_sample will replace this

    i = 0;
    if (debug("shifts")) {
        fprintf(stderr, "shifted: [");
        fprintf(stderr, "%"PRIu64"", samples[i++]);
        while (i <= p.k - 1U)
            fprintf(stderr, ", %"PRIu64"", samples[i++]);
        fprintf(stderr, "]\n");
    }
}

// rdtsc ticks at a constant rate, regardless of actual CPU frequency
// and so it can be converted to nanoseconds by using cpuid to get the CPU's base frequency
#define PROC_FREQ_LEAF 0x16
static uint64_t cycles_to_ns(uint64_t cycles) {
    static unsigned freq = 0;

    if (!freq) {
        unsigned b, c, d;
        __get_cpuid(PROC_FREQ_LEAF, &freq, &b, &c, &d);

        freq = freq / 100; // mhz to ghz * 10

        if (!freq) {
            fprintf(stderr, "could not determine processor base frequency, check leaf 0x16\n");
            abort();
        }

        if (debug("rdstc"))
            fprintf(stderr, "processor base frequency (ghz * 10): %u\n", freq);
    }

    return cycles * 10 / freq;            // 2.9 cycles per nanosecond
}

static uint64_t bench(struct bench_params const p, void (*fn)(void *args), void *args) {
    uint64_t samples[p.k];
    unsigned s = 0;

    if (p.prime_cache)
        (*fn)(args);

    do {
        bool uts = p.use_rdtsc;
        uint64_t start = now(uts);
        (*fn)(args);
        uint64_t elapsed = now(uts) - start;
        if (uts) elapsed = cycles_to_ns(elapsed);

        if (p.shift_samples) try_shift(samples, s, p);
        sort_samples(samples, add_sample(samples, s++, elapsed, p));
    } while (!has_converged(samples, s, p) && s < p.max_samples);

    if (debug("collection"))
        fprintf(stderr, "collected %d samples, result: %"PRIu64"\n", s, samples[0]);

    return samples[0];
}

struct read_data_args {
    volatile void *data;
    uint64_t n, stride;
};

#define define_read_data(name, T)                                       \
static void name##_read_data(void *args) {                              \
    struct read_data_args const *a = args;                              \
    volatile T *data = a->data;                                         \
    for (uint64_t i = 0; i < a->n; i += a->stride) data[i];             \
    _mm_lfence();                                                       \
}                                                                       \
                                                                        \
static void name##_read_data_sink(void *args) {                         \
    struct read_data_args const *a = args;                              \
    volatile T *data = a->data;                                         \
    volatile T sink = { 0 };                                            \
    T res = { 0 };                                                      \
    for (uint64_t i = 0; i < a->n; i += a->stride) res += data[i];      \
    sink = res;                                                         \
}

define_read_data(uint64, uint64_t)
define_read_data(avx2, __m256i)

static void (*benchmarks[])(void *args) = {
    [UINT64]      = uint64_read_data,
    [UINT64_SINK] = uint64_read_data_sink,
    [AVX2]        = avx2_read_data,
    [AVX2_SINK]   = avx2_read_data_sink
};

static size_t element_size(enum benchmark b) {
    switch (b) {
        case UINT64:
        case UINT64_SINK:
        default:
            return sizeof (uint64_t);
        case AVX2:
        case AVX2_SINK:
            return sizeof (__m256i);
    }
}

static char *benchmark_str(enum benchmark b) {
    switch (b) {
        default:
        case UINT64:      return "uint64";
        case UINT64_SINK: return "uint64_sink";
        case AVX2:        return "avx2";
        case AVX2_SINK:   return "avx2_sink";
    }
}

int main(int argc, char const *argv[]) { (void) argc;
    struct args args = {
        .benchmark = UINT64,
        .stride_interval = 2,
        .start_stride = 1,
        .end_stride = 32,
        .min_size_p2 = 10, // 2^10 bytes or 1 KB
        .max_size_p2 = 27,  // 2^27 bytes or 128 MB
        .shift_samples = 60,
        .prime_cache = true,
        .use_rdtsc = false
    };

    if (!parse_args(&args, "1.0.0", argv))
        return EXIT_FAILURE;

    if (debug("args"))
        debug_args(&args);

    if (!validate_args(&args))
        return EXIT_FAILURE;

    struct bench_params params = {
        .prime_cache = args.prime_cache,        // run the test before entering the timing loop to try and prime the cache
        .k = 5,                                 // require k samples
        .max_samples = 300,                     // give it 300 chances to converge
        .shift_samples = args.shift_samples,    // every n samples shift the minimum off (helps with convergence if early readings were fast)
        .denom = 100,                           // spread = min_value / denom + base_spread
        .base_spread = 2,                       // at least 2 nanoseconds
        .use_rdtsc = args.use_rdtsc
    };

    if (debug("bench_params"))
        debug_bench_params(&params);

    if (debug("benchmark"))
        fprintf(stderr, "running benchmark: %s\n", benchmark_str(args.benchmark));

    volatile void *data = malloc(1 << args.max_size_p2);
    if (!data) {
        fprintf(stderr, "data allocation failed\n");
        return EXIT_FAILURE;
    }

    for (unsigned size = 1 << args.max_size_p2; size >= 1U << args.min_size_p2; size >>= 1) {
        for (unsigned stride = args.start_stride; stride <= args.end_stride; stride += args.stride_interval) {
            struct read_data_args fargs = { data, size / element_size(args.benchmark), stride };
            uint64_t time = bench(params, benchmarks[args.benchmark], &fargs);
            printf("%u %u %"PRIu64"\n", stride, size, time);
        }

        printf("\n");
    }

    free((void *) data);

    return EXIT_SUCCESS;
}
