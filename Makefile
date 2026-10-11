CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: all test test-fieldmouse test-sublixel clean
all: build/rough-fb

build/rough-fb: src/main.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

test: build/rough-fb test-sublixel
	python3 tests/test_raster.py build/rough-fb

test-sublixel:
	$(MAKE) -C sublixel test

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
	$(MAKE) -C sublixel clean
