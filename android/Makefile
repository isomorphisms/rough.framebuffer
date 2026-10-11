# DEX-free Android NativeActivity build for MIRO A1 (ARMv7/A32) and
# MIRO C67 (AArch64/arm64-v8a); the same renderer/input implementation.
# ICK at the recorded current revision lacks qualified Android NativeActivity
# host-system integration for this C source closure, so this candidate uses
# the explicit Android NDK C compiler rather than an accidental host CC.
# Requires ANDROID_NDK_HOME, a selected NDK version and a Linux x86_64 build
# host. APK packaging/signing is a separate reviewed stage, not this target.
NDK_ROOT ?= $(ANDROID_NDK_HOME)
ANDROID_API ?= 21
ANDROID_ABI ?= armeabi-v7a
ifeq ($(ANDROID_ABI),armeabi-v7a)
ANDROID_TARGET := armv7a-linux-androideabi
ANDROID_LINK_ALIGNMENT :=
else ifeq ($(ANDROID_ABI),arm64-v8a)
ANDROID_TARGET := aarch64-linux-android
ANDROID_LINK_ALIGNMENT := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
else ifeq ($(ANDROID_ABI),x86_64)
ANDROID_TARGET := x86_64-linux-android
ANDROID_LINK_ALIGNMENT := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
else
$(error Unsupported Android ABI $(ANDROID_ABI); supported: armeabi-v7a, arm64-v8a, x86_64)
endif
NDK_TOOLCHAIN := $(NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
ANDROID_CC := $(NDK_TOOLCHAIN)/$(ANDROID_TARGET)$(ANDROID_API)-clang
ANDROID_READELF := $(NDK_TOOLCHAIN)/llvm-readelf
GLUE_DIR := $(NDK_ROOT)/sources/android/native_app_glue
BUILD_DIR := ../build/android/$(ANDROID_ABI)
ANDROID_LIB := $(BUILD_DIR)/libroughart.so

.PHONY: all check-ndk elf-report clean
all: $(ANDROID_LIB)

check-ndk:
	@test -n "$(NDK_ROOT)" || { echo "Android NDK root is not declared" >&2; exit 1; }
	@test -x "$(ANDROID_CC)" || { echo "Missing selected Android NDK Clang: $(ANDROID_CC)" >&2; exit 1; }
	@test -f "$(GLUE_DIR)/android_native_app_glue.c" || { echo "Missing NDK native app glue" >&2; exit 1; }
	@test -x "$(ANDROID_READELF)" || { echo "Missing NDK llvm-readelf" >&2; exit 1; }

$(BUILD_DIR)/android_native_app_glue.o: check-ndk
	@mkdir -p $(BUILD_DIR)
	$(ANDROID_CC) -std=c11 -O2 -fPIC -Wall -Wextra -Werror -Wno-unused-parameter -isystem $(GLUE_DIR) -c $(GLUE_DIR)/android_native_app_glue.c -o $@

$(ANDROID_LIB): ../android/native/rough_android.c ../src/touch.c ../src/touch.h ../src/surface.c ../src/surface.h ../src/framebuffer.c ../src/framebuffer.h ../src/present.c ../src/present.h ../src/surface_aa.c ../src/surface_aa.h $(BUILD_DIR)/android_native_app_glue.o
	$(ANDROID_CC) -std=c11 -O2 -fPIC -shared -Wall -Wextra -Werror -pedantic \
	    -isystem $(GLUE_DIR) -I../src -I../android/native \
	    ../android/native/rough_android.c ../src/touch.c ../src/surface.c ../src/framebuffer.c ../src/present.c ../src/surface_aa.c \
	    $(BUILD_DIR)/android_native_app_glue.o \
	    -Wl,--no-undefined -Wl,-soname,libroughart.so $(ANDROID_LINK_ALIGNMENT) \
	    -landroid -llog -lm -o $@

elf-report: $(ANDROID_LIB)
	$(ANDROID_READELF) -h $(ANDROID_LIB)
	$(ANDROID_READELF) -s $(ANDROID_LIB)
	$(ANDROID_READELF) -d $(ANDROID_LIB)

clean:
	rm -rf $(BUILD_DIR)
