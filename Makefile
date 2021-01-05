TIME_TEST := time_test
TSC := tsc
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

absTime: absTime.hs
	$(GHC) -o $@ $^

.PHONY: clean
clean:
	rm -rf *.s *.o $(TIME_TEST) $(TSC) *.dSYM
	rm -rf *.hi *.o absTime

