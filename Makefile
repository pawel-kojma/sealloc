CC=clang
CFLAGS=-std=c17
INCLUDES=-I./include
SRC=./src

all: internal_allocator.o logging.o

internal_allocator.o: $(SRC)/internal_allocator.c
	$(CC) $(CFLAGS) -c -o internal_allocator.o $(INCLUDES) $(SRC)/internal_allocator.c

logging.o: $(SRC)/logging.c
	$(CC) $(CFLAGS) -c -o logging.o $(INCLUDES) $(SRC)/logging.c

clean:
	rm -rf *.o *.a *.elf

.PHONY: clean

