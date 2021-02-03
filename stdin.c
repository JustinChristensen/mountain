#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

#define ONE_SEC_NS 1000000000
static uint64_t now() {
    struct timespec ts;

    // CLOCK_MONOTONIC on OSX only supports microsecond precision, and each nanosecond counts
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts)) {
        fprintf(stderr, "error reading current time\n");
        abort();
    }

    return ts.tv_sec * ONE_SEC_NS + ts.tv_nsec;
}

static uint64_t bench(void (*fn)(void *args), void *args) {
    uint64_t start = now();
    (*fn)(args);
    return now() - start;
}

struct read_stdin_args {
    int bufsiz;
};

void read_stdin(void *args) {
    struct read_stdin_args *a = args;
    volatile char c;
    for (int i = 0; i < a->bufsiz; i++) c = getchar();
}

int main(int argc, char const *argv[]) {
    (void) argc, (void) argv;
    char *env_p2_bufsiz = getenv("BUFSIZ");
    long p2_bufsiz = 10; // 1 KiB

    if (env_p2_bufsiz) {
        char *end = NULL;

        p2_bufsiz = strtol(env_p2_bufsiz, &end, 10);

        if (env_p2_bufsiz == end) {
            fprintf(stderr, "invalid buffer size given: %s\n", env_p2_bufsiz);
            exit(EXIT_FAILURE);
        }

        if (p2_bufsiz < 0 || p2_bufsiz > 30) {
            fprintf(stderr, "buffer size out of range: [0, 30]\n");
            exit(EXIT_FAILURE);
        }
    }

    int bufsiz = 1 << p2_bufsiz;
    if (getenv("DEBUG"))
        fprintf(stderr, "using size: %d\n", bufsiz);
    setvbuf(stdin, NULL, _IOFBF, bufsiz);

    struct read_stdin_args fargs = { bufsiz };
    printf("%"PRIu64"\n", bench(read_stdin, &fargs));

    return EXIT_SUCCESS;
}
