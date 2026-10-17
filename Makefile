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

build/test-present: tests/test_present.c src/present.c src/present.h src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_present.c src/present.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-rotation-grab: tests/test_rotation_grab.c src/touch.c src/touch.h src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_rotation_grab.c src/touch.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

build/test-surface-aa: tests/test_surface_aa.c src/surface_aa.c src/surface_aa.h src/surface.c src/surface.h src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc tests/test_surface_aa.c src/surface_aa.c src/surface.c src/framebuffer.c -o $@ $(LDLIBS)

# Reference-only C11 visual probe for the test APK (not ICK acceptance).
build/test-subpixel-demo-reference: tests/test_subpixel_demo.c src/subpixel_demo.c src/subpixel_demo.h subpixel/src/curve_patch.c subpixel/src/curve_patch_5x5.c subpixel/include/subpixel/curve_patch.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isrc -Isubpixel/include tests/test_subpixel_demo.c src/subpixel_demo.c subpixel/src/curve_patch.c subpixel/src/curve_patch_5x5.c -o $@ $(LDLIBS)

# Host differential check; product ICK compilation is in Subpixel PR #8.
build/test-subpixel5-reference: subpixel/tests/test_curve_patch_5x5.c subpixel/src/curve_patch.c subpixel/src/curve_patch_5x5.c subpixel/include/subpixel/curve_patch.h
	@mkdir -p build
	$(CC) $(CFLAGS) -Isubpixel/include subpixel/tests/test_curve_patch_5x5.c subpixel/src/curve_patch.c subpixel/src/curve_patch_5x5.c -o $@ $(LDLIBS)

test: build/test-subpixel-demo-reference build/test-subpixel5-reference build/rough-fb build/rough-surface build/test-api build/test-surface build/test-touch build/test-present build/test-rotation-grab build/test-surface-aa
	CC=$(CC) sh ci/test-subpixel-naming.sh
	./build/test-subpixel-demo-reference
	./build/test-subpixel5-reference
	./build/test-api
	./build/test-surface
	./build/test-touch
	./build/test-rotation-grab
	./build/test-present
	./build/test-surface-aa
	python3 tests/test_raster.py build/rough-fb

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
