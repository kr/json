CFLAGS ?= -Werror -Wall -Wformat=2

all: check

json.o: json.c json.h

.PHONY: check
check: ct/_ctcheck
	ct/_ctcheck

ct/ct.o: ct/ct.h

test.o: ct/ct.h json.h

ct/_ctcheck: ct/_ctcheck.o ct/ct.o json.o test.o

ct/_ctcheck.c: test.o ct/gen
	ct/gen test.o > $@.part
	mv $@.part $@

.PHONY: clean
clean:
	rm -f *.o ct/_* ct/*.o
