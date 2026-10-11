CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS ?= -lm

.PHONY: all test clean
all: build/test_curve_patch

build/test_curve_patch: src/curve_patch.c include/sublixel/curve_patch.h tests/test_curve_patch.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/curve_patch.c tests/test_curve_patch.c -o $@ $(LDLIBS)

test: build/test_curve_patch
	./build/test_curve_patch

clean:
	rm -rf build
