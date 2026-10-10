CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: all test test-fieldmouse clean
all: build/rough-fb build/rough-surface

build/rough-fb: src/main.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

build/rough-surface: src/surface_main.c src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) src/surface_main.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-api: tests/test_api.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_api.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-surface: tests/test_surface.c src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_surface.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-touch: tests/test_touch.c src/touch.c src/touch.h src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_touch.c src/touch.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

test: build/rough-fb build/rough-surface build/test-api build/test-surface build/test-touch
	./build/test-api
	./build/test-surface
	./build/test-touch
	python3 tests/test_raster.py build/rough-fb

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
