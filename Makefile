# Native Rough framebuffer consumers compile only through source-verified ICK.
# Historical build/ and tests are retained; no stock C compiler fallback.
ICK_CC ?=
ICK_SOURCE_DIR ?=
ICK_LINK_FLAGS ?= -fno-link-libatomic
ICK_CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Werror -pedantic -ffp-contract=off -fno-fast-math
LDLIBS = -lm
ICK_SOURCE_SHA = c61e448251744a2f40ad743ebef1a027bdcd2f9d
ICK_REFERENCE_SHA = 6294f1d9e7536e5ffcde09d1528c918d63abfef5

.PHONY: all test test-fieldmouse check-ick clean
all: build/rough-fb

check-ick:
	@test -n "$(ICK_CC)" && test -n "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: pinned ICK_CC and ICK_SOURCE_DIR required" >&2; exit 3; }
	@test -x "$(ICK_CC)" && test -d "$(ICK_SOURCE_DIR)" || { echo "BLOCKED: owned ICK executable/source missing" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)" rev-parse HEAD)" = "$(ICK_SOURCE_SHA)" || { echo "BLOCKED: ICK revision mismatch" >&2; exit 3; }
	@test "$$(git -C "$(ICK_SOURCE_DIR)/gcc" rev-parse HEAD)" = "$(ICK_REFERENCE_SHA)" || { echo "BLOCKED: ICK reference revision mismatch" >&2; exit 3; }
	@case "$(ICK_CC)" in */ick-host-stage/bin/gcc) ;; *) echo "BLOCKED: refuse stock gcc/clang or fake compiler" >&2; exit 3 ;; esac
	@test "$$("$(ICK_CC)" -dumpmachine)" = x86_64-pc-linux-gnu || { echo "BLOCKED: wrong ICK host target" >&2; exit 3; }
	@cc1="$$("$(ICK_CC)" -print-prog-name=cc1)"; test -x "$$cc1" || { echo "BLOCKED: ICK cc1 missing" >&2; exit 3; }

build/rough-fb: src/main.c src/framebuffer.c src/framebuffer.h check-ick
	@mkdir -p build
	"$(ICK_CC)" $(ICK_LINK_FLAGS) $(ICK_CFLAGS) src/main.c src/framebuffer.c -o $@ $(LDLIBS)

test: build/rough-fb
	python3 tests/test_raster.py build/rough-fb

test-fieldmouse: build/rough-fb
	FIELD_MOUSE=$(FIELD_MOUSE) python3 tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
