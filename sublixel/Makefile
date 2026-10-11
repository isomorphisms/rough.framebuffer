CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS ?= -lm

.PHONY: all test test-ick test-reference clean

# ICK is the product compiler. Missing ICK is a hard failure, not a
# permission to fall back silently to GCC or Clang.
all: test-ick
test: test-ick
test-ick:
	sh qualification/test-ick.sh

# Differential/reference-only testing. Never claim this is ICK acceptance.
build/reference_test_curve_patch: src/curve_patch.c include/sublixel/curve_patch.h tests/test_curve_patch.c
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude src/curve_patch.c tests/test_curve_patch.c -o $@ $(LDLIBS)

test-reference: build/reference_test_curve_patch
	./build/reference_test_curve_patch

clean:
	rm -rf build
