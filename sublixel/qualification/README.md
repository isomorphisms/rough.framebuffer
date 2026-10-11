# Mandatory owned toolchain boundary

Sublixel is part of rough.framebuffer. All consumer C translation units
(including test programs and the existing framebuffer CLI) compile exclusively
through pinned, source-built **ICK**. No GCC/Clang differential build lanes,
`CC ?= cc` defaults, fallback NDK builds, or reference targets remain in
the native build recipes.

The exact source pin is [dilapidated-shed/ick@c61e448251744a2f40ad743ebef1a027bdcd2f9d](https://github.com/dilapidated-shed/ick/tree/c61e448251744a2f40ad743ebef1a027bdcd2f9d),
with its immutable GCC source reference pinned at
`6294f1d9e7536e5ffcde09d1528c918d63abfef5`.
The program `qualification/test-ick.sh` checks both commits, ICK driver
target and installed cc1 identity. ICK then compiles the following five sources:

- `sublixel/src/curve_patch.c`
- `sublixel/tests/test_curve_patch.c`
- `src/framebuffer.c`
- `src/main.c`
- `tests/test_raster_ick.c`

It checks each ELF64 object, links the test executables and the existing
`build/rough-fb` program, executes both native test suites, and reports
the selected ICK binary, cc1 and artifact hashes. Missing ICK refuses all
builds. The CI workflow explicitly exercises this negative condition.

```sh
ICK_CC=/absolute/ick/stage/bin/x86_64-linux-gnu-gcc \
ICK_SOURCE_DIR=/absolute/pinned/ick/checkout \
HOST_LINKER=/usr/bin/x86_64-linux-gnu-gcc \
make test
```

**Stage-zero and linking exceptions:** ICK remains GCC-derived and is not
yet self-hosting. Building the owned compiler from its pinned source uses
system build-essential, including GCC/G++, and the Linux object linker
uses the separately declared system compiler *only as a linker driver* for
CRT/libc/libm. It never receives a project source file. This is honest
ICK-compiled application code, not proof of an all-owned compiler bootstrap
or all-ICK runtime/linker.

The earlier GCC/Clang consumer builds are superseded evidence only.
Legacy Python raster tests are retained in the tree but are not part of
the native acceptance path; `tests/test_raster_ick.c` exercises the same
pixel/render boundary with ICK-compiled C. An optional Field Mouse test
requires an explicit Ithon and has not been qualified under it.

ICKY is not yet integrated as ICK's full C parser. IKE, Ithon and ILua
are selected only for the stages they actually support; they must never
be asserted to support this C11 code without execution evidence. Android
packaging, phone runtime verification, registered Flexible Pipes dispatch
and independent ai-ci acceptance remain separate, currently unmet gates.
