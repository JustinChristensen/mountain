TIME_TEST := time_test
TSC := tsc
ABS_TIME := absTime
MOUNTAIN := mountain
GNUPLOT := gnuplot
CFLAGS += -Wall -Wextra -g -O3
GHC := ghc

$(MOUNTAIN): CFLAGS += -mavx2
$(MOUNTAIN):
$(TIME_TEST): CFLAGS += -mavx2
$(TIME_TEST):
$(TSC):

%:: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.s:: CFLAGS += -S
%.s:: %.c
	$(CC) $(CFLAGS) -o $@ $^

$(ABS_TIME): absTime.hs
	$(GHC) -o $@ $^

run.txt: $(MOUNTAIN)
	./$(MOUNTAIN) -b avx2 > $@

.PHONY: mountain.svg
mountain.svg: run.txt
	$(GNUPLOT) splot.gnu > $@

.PHONY: all
all: $(TIME_TEST) $(TSC) $(ABS_TIME)

.PHONY: run-debug
run-debug:
	STDBUF1=L STDBUF2=L DEBUG=* ./$(MOUNTAIN)

.PHONY: clean
clean:
	rm -rf *.s *.o $(TIME_TEST) $(TSC) $(MOUNTAIN) *.dSYM
	rm -rf *.hi *.o absTime *.txt *.jpg *.png

