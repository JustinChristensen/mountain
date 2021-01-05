PROG := time_test
EXT :=
CFLAGS := -Wall -Wextra -O -g

$(PROG): time.c
	$(CC) $(CFLAGS) -o $@$(EXT) $^ 

# -fverbose-asm
$(PROG).s: CFLAGS += -S 
$(PROG).s: EXT := .s
$(PROG).s: $(PROG)

.PHONY: clean
clean:
	rm -rf *.s *.o $(PROG) *.dSYM
	rm -rf *.hi *.o absTime

