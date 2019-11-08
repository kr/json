LIB=libjson.a
CFLAGS ?= -Werror -Wall -Wformat=2 -ansi -fsanitize=undefined
LDFLAGS ?= -fsanitize=undefined

all: $(LIB) check

json.o: json.c json.h

$(LIB): json.o
	ar rvc $(LIB) json.o

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
	rm -f *.o ct/_* ct/*.o $(LIB)

.PHONY: install
install: $(LIB)
	cp $(LIB) /usr/local/lib
	cp json.h /usr/local/include
