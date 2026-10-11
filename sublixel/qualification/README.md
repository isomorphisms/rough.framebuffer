# ICK compiler qualification and fallback policy

Sublixel is an ordinary C11 geometry library; ICK (not a host GCC/Clang executable) is the designated production C compiler. The earlier test run on PR #6 was a **reference compiler** check only.

The `test-ick` target uses the complete **owned ICK** C frontend from [dilapidated-shed/ick](https://github.com/dilapidated-shed/ick) at source commit `c61e448251744a2f40ad743ebef1a027bdcd2f9d`, with the GCC reference at `6294f1d9e7536e5ffcde09d1528c918d63abfef5`. The workflow materializes its source under ICK's own `ick/materialize.sh`, builds the native x86_64 C compiler, authenticates the installed `cc1`, and compiles **both** Sublixel translation units using the ICK driver exclusively. The compiler's own hosted bootstrap uses build-essential; those host compilers **do not compile Sublixel source**.

After ICK emits ELF64 x86_64 objects, the named Linux host linker consumes those objects with libc/libm and runs the entire test executable. This is **ICK C object generation and Linux functional validation**, not an ICK Android sysroot, ICK-only runtime/linker or device APK qualification. Toolchain paths are mandatory and missing ICK always fails closed; there is no environment auto-fallback. The driver, `cc1`, object and executable hashes are printed as receipt material.

To execute after separately building the pinned compiler:

```sh
ICK_CC=/absolute/path/to/ick/stage/bin/x86_64-linux-gnu-gcc \
ICK_SOURCE_DIR=/absolute/path/to/ick/checkout \
HOST_LINKER=/usr/bin/x86_64-linux-gnu-gcc \
make test-sublixel
```

`make -C sublixel test-reference` is deliberately a **diagnostic-only** GCC/Clang control and never substitutes for `test-ick`. The existing rough.framebuffer renderer retains its own separate C-reference tests.

**Open qualifications:** ICKY is not integrated as a replacement general C source parser (see ICK PR #89); this code uses ordinary C11 tokens. Android source/header, ELF32 A32/ARMv7, C67 AArch64, linker/sysroot, ABI and physical runtime acceptance remain separate and are not inferred from Linux x86_64 test success. The ICK x86_64 lane does not itself merge the work; Flexible Pipes registered dispatch and independent ai-ci remain unsatisfied.
