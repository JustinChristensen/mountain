TIME_TEST := time_test
TSC := tsc
ABS_TIME := absTime
EXT :=
CFLAGS := -Wall -Wextra -O -g
GHC := ghc

$(TIME_TEST):
$(TSC):

%:: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.s:: CFLAGS += -S
%.s:: %.c
	$(CC) $(CFLAGS) -o $@ $^

$(ABS_TIME): absTime.hs
	$(GHC) -o $@ $^

.PHONY:
all: $(TIME_TEST) $(TSC) $(ABS_TIME)

.PHONY: clean
clean:
	rm -rf *.s *.o $(TIME_TEST) $(TSC) *.dSYM
	rm -rf *.hi *.o absTime

