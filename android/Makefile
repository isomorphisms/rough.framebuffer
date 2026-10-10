# DEX-free Android NativeActivity build for MIRO A1 (ARMv7/A32).
# ICK at the recorded current revision lacks qualified Android NativeActivity
# host-system integration for this C source closure, so this candidate uses
# the explicit Android NDK C compiler rather than an accidental host CC.
# Requires ANDROID_NDK_HOME, a selected NDK version and a Linux x86_64 build
# host. APK packaging/signing is a separate reviewed stage, not this target.
NDK_ROOT ?= $(ANDROID_NDK_HOME)
ANDROID_API ?= 21
NDK_TOOLCHAIN := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
ANDROID_CC := $(NDK_TOOLCHAIN)/armv7a-linux-androideabi$(ANDROID_API)-clang
ANDROID_READELF := $(NDK_TOOLCHAIN)/llvm-readelf
GLUE_DIR := $(NDK_ROOT)/sources/android/native_app_glue
BUILD_DIR := ../build/android/armeabi-v7a
ANDROID_LIB := $(BUILD_DIR)/libroughart.so

.PHONY: all check-ndk elf-report clean
all: $(ANDROID_LIB)

check-ndk:
	@test -n "$(NDK_ROOT)" || { echo "Android NDK root is not declared" >&2; exit 1; }
	@test -x "$(ANDROID_CC)" || { echo "Missing ARMv7 NDK Clang: $(ANDROID_CC)" >&2; exit 1; }
	@test -f "$(GLUE_DIR)/android_native_app_glue.c" || { echo "Missing NDK native app glue" >&2; exit 1; }
	@test -x "$(ANDROID_READELF)" || { echo "Missing NDK llvm-readelf" >&2; exit 1; }

$(BUILD_DIR)/android_native_app_glue.o: check-ndk
	@mkdir -p $(BUILD_DIR)
	$(ANDROID_CC) -std=c11 -O2 -fPIC -Wall -Wextra -Werror -Wno-unused-parameter -isystem $(GLUE_DIR) -c $(GLUE_DIR)/android_native_app_glue.c -o $@

$(ANDROID_LIB): ../android/native/rough_android.c ../src/touch.c ../src/touch.h ../src/surface.c ../src/surface.h ../src/framebuffer.c ../src/framebuffer.h $(BUILD_DIR)/android_native_app_glue.o
	$(ANDROID_CC) -std=c11 -O2 -fPIC -shared -Wall -Wextra -Werror -pedantic \
	    -isystem $(GLUE_DIR) -I../src -I../android/native \
	    ../android/native/rough_android.c ../src/touch.c ../src/surface.c ../src/framebuffer.c \
	    $(BUILD_DIR)/android_native_app_glue.o \
	    -Wl,--no-undefined -Wl,-soname,libroughart.so \
	    -landroid -llog -lm -o $@

elf-report: $(ANDROID_LIB)
	$(ANDROID_READELF) -h $(ANDROID_LIB)
	$(ANDROID_READELF) -s $(ANDROID_LIB)
	$(ANDROID_READELF) -d $(ANDROID_LIB)

clean:
	rm -rf $(BUILD_DIR)
