CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS ?= -lm

COMMON = src/curve_patch.c src/curve_patch_5x5.c
HEADER = include/sublixel/curve_patch.h

.PHONY: all test test-ick test-reference clean

# ICK is the product compiler. The default test never uses GCC/Clang fallback.
all: test-ick
test: test-ick
test-ick:
	sh qualification/test-ick.sh

# GCC and Clang are differential/reference controls only.
build/reference_test_curve_patch: $(COMMON) $(HEADER) tests/test_curve_patch.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(COMMON) tests/test_curve_patch.c -o $@ $(LDLIBS)

build/reference_test_curve_patch_5x5: $(COMMON) $(HEADER) tests/test_curve_patch_5x5.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude $(COMMON) tests/test_curve_patch_5x5.c -o $@ $(LDLIBS)

test-reference: build/reference_test_curve_patch build/reference_test_curve_patch_5x5
	./build/reference_test_curve_patch
	./build/reference_test_curve_patch_5x5

clean:
	rm -rf build
