TIME_TEST := time_test
TSC := tsc
ABS_TIME := absTime
MOUNTAIN := mountain
EXT :=
CFLAGS += -Wall -Wextra -g
GHC := ghc

$(TIME_TEST):
$(TSC):
$(MOUNTAIN):

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
	rm -rf *.s *.o $(TIME_TEST) $(TSC) $(MOUNTAIN) *.dSYM
	rm -rf *.hi *.o absTime

