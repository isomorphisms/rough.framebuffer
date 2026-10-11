CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: all test test-fieldmouse test-sublixel test-sublixel-reference clean
all: build/rough-fb

build/rough-fb: src/main.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

# The Sublixel acceptance lane requires a source-built pinned ICK.
test: build/rough-fb test-sublixel
	python3 tests/test_raster.py build/rough-fb

test-sublixel:
	$(MAKE) -C sublixel test-ick

# GCC and Clang remain explicit non-production differential controls.
test-sublixel-reference:
	$(MAKE) -C sublixel test-reference

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
	$(MAKE) -C sublixel clean
