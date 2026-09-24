CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2

.PHONY: all test clean

all: test

build/test_qioman: src/qioman.c tests/test_qioman.c include/qioman.h
	mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/qioman.c tests/test_qioman.c -o $@

test: build/test_qioman
	./build/test_qioman

clean:
	rm -f build/test_qioman
