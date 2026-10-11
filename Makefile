# ICK is the only permitted source compiler. Missing ICK blocks all targets.
.PHONY: all test test-sublixel test-fieldmouse clean

all test test-sublixel:
	sh sublixel/qualification/test-ick.sh

# This optional integration explicitly requires an owned Ithon interpreter.
# No Python 3 or Node fallback. Ithon compatibility is not yet qualified.
test-fieldmouse: test-sublixel
	@set -eu; \
	  : "${FIELD_MOUSE:?FIELD_MOUSE must name the owned interpreter}"; \
	  : "${ITHON:?ITHON must name an explicitly qualified interpreter}"; \
	  FIELD_MOUSE="$$FIELD_MOUSE" "$$ITHON" tests/test_fieldmouse.py build/rough-fb

clean:
	rm -rf build
	$(MAKE) -C sublixel clean
