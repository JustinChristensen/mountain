#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

enum arg_type {
    UNKNOWN_ARG,
    MISSING_VAL,
    INVALID_VAL,
    HELP,
    VERSION,
    STRIDE_INTERVAL,
    START_STRIDE,
    END_STRIDE,
    MIN_SIZE,
    MAX_SIZE
};

struct arg {
    enum arg_type type;
    char flag[128];
    union {
        uint8_t u8;
        char *s;
    };
};

struct args {
    uint8_t stride_interval, // interval=[1,n] where interval divides max_stride
            start_stride,    // stride=[1,n] where start < end
            end_stride,
            min_size_p2,     // size=2^n where n=[10,27] and min_size < max_size
            max_size_p2;
};

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

static void setflag(char *flag, char const *s, char const *e) {
    size_t const n = e - s;
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
        while (*s && *s != '=') s++;
        if (strncmp(lflag, f, s - f)) return false;

        arg->type = type;
        setflag(arg->flag, a, s);
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
    fprintf(handle, optfmt, "-n,--stride-interval", "Interval to increase the stride by (+= 2)");
    fprintf(handle, optfmt, "-s,--start-stride", "Starting stride (1)");
    fprintf(handle, optfmt, "-e,--end-stride", "Ending stride (32)");
    fprintf(handle, optfmt, "-i,--min-size", "Minimum size as a power of two (2^n where n = 10 or 1 KB)");
    fprintf(handle, optfmt, "-a,--max-size", "Maximum size as a power of two (2^n where n = 27 or 128 MB)");
    fprintf(handle, "\n");
}

static char const **parse_arg(struct arg *arg, char const **argv) {
    bool found_arg =
        _parse_arg("help", 'h', HELP, NULL, arg, &argv) ||
        _parse_arg("version", 'v', VERSION, NULL, arg, &argv) ||
        _parse_arg("stride-interval", 'n', STRIDE_INTERVAL, uint8_val, arg, &argv) ||
        _parse_arg("start-stride", 's', START_STRIDE, uint8_val, arg, &argv) ||
        _parse_arg("end-stride", 'e', END_STRIDE, uint8_val, arg, &argv) ||
        _parse_arg("min-size", 'i', MIN_SIZE, uint8_val, arg, &argv) ||
        _parse_arg("max-size", 'a', MAX_SIZE, uint8_val, arg, &argv);

    if (!found_arg) {
        char const *s = *argv;
        while (*s && *s != '=') s++;
        arg->type = UNKNOWN_ARG;
        setflag(arg->flag, *argv, s);
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
            case STRIDE_INTERVAL:   args->stride_interval = arg.u8; break;
            case START_STRIDE:      args->start_stride = arg.u8; break;
            case END_STRIDE:        args->end_stride = arg.u8; break;
            case MIN_SIZE:          args->min_size_p2 = arg.u8; break;
            case MAX_SIZE:          args->max_size_p2 = arg.u8; break;
            case VERSION:
                print_version(stdout, version);
                exit(0);
                break;
            case HELP:
                print_usage(stdout, prog);
                exit(0);
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

int main(int __attribute__((unused)) argc, char const *argv[]) {
    struct args args = {
        .stride_interval = 2,
        .start_stride = 1,
        .end_stride = 32,
        .min_size_p2 = 10, // 2^10 bytes or 1 KB
        .max_size_p2 = 27  // 2^27 bytes or 128 MB
    };

    if (!parse_args(&args, "1.0.0", argv))
        return EXIT_FAILURE;

    if (getenv("DEBUG")) {
        fprintf(stderr, "%s\n", "args:");
        fprintf(stderr,
            "  stride_interval = %hhu\n"
            "  start_stride = %hhu\n"
            "  end_stride = %hhu\n"
            "  min_size_p2 = %hhu\n"
            "  max_size_p2 = %hhu\n",
            args.stride_interval,
            args.start_stride,
            args.end_stride,
            args.min_size_p2,
            args.max_size_p2);
    }

    return EXIT_SUCCESS;
}
