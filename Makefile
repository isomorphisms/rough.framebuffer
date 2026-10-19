CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: all test test-fieldmouse clean
all: build/rough-fb

build/rough-fb: src/main.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-api: tests/test_api.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_api.c src/framebuffer.c -o $@ $(LDLIBS)

test: build/rough-fb build/test-api
	./build/test-api
	python3 tests/test_raster.py build/rough-fb

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
