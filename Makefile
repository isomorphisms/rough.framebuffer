# PRODUCT: ICK is required, with no compiler fallback.
# HOST_LINKER is explicitly allowed for object-only Linux libc/libm linking.
# REFERENCE_CC and CC select only named reference/differential targets.
REFERENCE_CC ?= cc
REFERENCE_CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm

.PHONY: all test test-fieldmouse test-sublixel test-reference \
    test-fieldmouse-reference test-sublixel-reference reference clean FORCE
all: build/rough-fb

# FORCE prevents a cached executable from bypassing the compiler identity gate.
build/rough-fb: FORCE src/main.c src/framebuffer.c src/framebuffer.h qualification/build-ick.sh
	sh qualification/build-ick.sh

test: build/rough-fb test-sublixel
	python3 tests/test_raster.py build/rough-fb

test-sublixel:
	$(MAKE) -C sublixel test-ick

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

# Explicit reference controls, never production or ICK qualification.
build/rough-fb-reference: src/main.c src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(REFERENCE_CC) $(REFERENCE_CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

reference: build/rough-fb-reference

test-reference: reference test-sublixel-reference
	python3 tests/test_raster.py build/rough-fb-reference

test-sublixel-reference:
	$(MAKE) -C sublixel test-reference

test-fieldmouse-reference: reference
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb-reference

clean:
	rm -rf build
	$(MAKE) -C sublixel clean
