# Smaller C build (experiment)

This is an evaluation of [Smaller C](https://github.com/alexfru/SmallerC)
(2-clause BSD) as an alternative to Open Watcom for building `df.exe`,
with the size of the DOS executable as the main measure. Open Watcom
remains the default compiler and the only one used for releases. The
Smaller C build is opt-in and produces `dfsc.exe`.

## Result

| Build | Compiler | Library | CPU | `.exe` bytes | vs Watcom |
| --- | --- | --- | --- | ---: | ---: |
| `df.exe` (default) | Open Watcom 2.0 | clibs | 8086 | **14,938** | — |
| `dfsc.exe` | Smaller C 1.0.2 | `lcds.a` rebuilt for 8086 | 8086 | **13,890** | −1,048 (−7.0%) |

Both rows are the same program, version 0.01, and both pass the same
tests (see [Behaviour](#behaviour)).

**Smaller C's executable is 7% smaller, but its build is much slower
on an 8086 and needs DOS 3 and 76 KB of memory.** See
[Behaviour](#behaviour). The size saving is real but small, so Watcom
stays the default.

Where the bytes go (code only, from the object files and linker maps):

| | Watcom | Smaller C |
| --- | ---: | ---: |
| `main.c`, `df.c`, `options.c`, `drives.c` | 1,129 | 2,186 |
| capacity arithmetic and formatting (`format.c`) | 1,008 | 2,651 |
| `int86` (`smallerc/i86.c` for Smaller C) | (in library) | 94 |
| 64-bit helpers (`i8m086`, `i8d086`) | 1,317 | (in `format.c`) |
| rest of the C library and startup | 9,708 | 7,216 |
| **total code** | 13,162 | 12,147 |

Smaller C's code generator, which does no optimization, makes the
program twice as large as Watcom's `-os` output. Its library is so
much smaller that the executable still comes out ahead. For
dos-which16, Smaller C was 2.9% larger (see its SMALLERC.md). One
reason df does better is that Watcom's 64-bit multiply and divide
helpers alone are 1.3 KB. Smaller C's `format.c`, with its own 64-bit arithmetic, is
only 1.6 KB larger than Watcom's.

## 64-bit arithmetic

Smaller C's 16-bit target has **no `long` or `long long`**, so the
`unsigned long long` that df's capacity arithmetic uses (up to
65535³ bytes, about 2⁴⁸) does not compile. This is the one change
that could not be avoided:

- `src/format.h` makes `dfsize_t` a structure of four 16-bit words
  when compiled by Smaller C (`__SMALLER_C__`). Watcom and the host
  still get `unsigned long long`, and Watcom's `df.exe` is
  byte-for-byte the same as before.
- `smallerc/format.c` replaces `src/format.c` in the Smaller C build.
  It has the same functions, with the arithmetic done by a few small
  helpers (add, subtract, compare, multiply by a 16-bit value by shift
  and add, 64-by-64 division by shift and subtract). Each function
  follows `src/format.c` expression by expression, so they give the
  same results, wrapping modulo 2⁶⁴ in the same places. It is portable
  C (no `asm`) so that the host can test it.
- The structure is passed and returned by value, as `dfsize_t` was.
  Smaller C supports that. Its code for a structure argument uses
  `push byte 0`, which `8086.inc` did not handle, so it now does (see
  [What it took](#what-it-took)).

The helpers are small but **slow**: they work a bit at a time, and the
decimal conversion divides the whole 64-bit value once per digit.

Checks of the arithmetic:

- `make unit-smallerc` builds `tests/test_format.c` and
  `tests/test_df.c` on the host against `smallerc/format.c`
  (`-DDFSIZE_WORDS`). These are the same tests as `make unit`, so they
  include values up to 2⁶⁴−1. `test_format.c` now converts sizes with
  `D()` and `N()` so that it compiles with either `dfsize_t`.
- `make test-rows` builds `tests/rows.c` with the host compiler, Watcom
  and Smaller C. The program prints both kinds of row (1 KiB and `-h`)
  for 1,696 free-space query results: every combination of 0, 1, 2,
  512, 32768 and 65535 in each field, then 400 pseudo-random ones. The
  two DOS builds run under DOSBox-X and must print exactly what the
  host build prints (5,088 lines). Both do. This is the check of each
  compiler's own code for the arithmetic, which `tests/run.sh` cannot
  give because DOSBox-X's drive sizes are fixed.

## Behaviour

Both builds pass the full DOS test suite (`tests/run.sh`, 82 checks of
output and exit status), all host unit tests and `make test-rows`.
Error messages come from the same `fprintf` calls. The tests do not
check them, because DOS cannot redirect standard error.

Differences found in the sanity check (DOSBox-X):

- **Speed on an 8086.** With DOSBox-X set to an 8086 at about
  original-PC speed (`cputype=8086`, `cycles=fixed 400`, a rough
  equivalent of a 4.77 MHz PC), one `df` of three drives took about
  **11 seconds** with `dfsc.exe` and 0.2 seconds with `df.exe`. `-h` was
  no faster. On a 286 or later, or with `cycles=max`, the delay is not
  noticeable. The cause is `smallerc/format.c`'s bit-at-a-time
  helpers, not the compiler. Following the issue, this was not
  optimized. A trial that divided with the 8086's `div` instruction
  (16 lines of inline `asm`) took a plain `df` down to 0.9 s and `-h`
  to about 8 s, and added 4 bytes. The size comparison does not depend
  on this.
- **DOS version.** Smaller C's startup code refuses to run before DOS
  3.0 (`DOS 3+ required!`). The Watcom build runs on DOS 2.11.
- **Memory.** A Smaller C small-model program always takes a full 64 KB
  data segment, so the MZ header asks for about 76 KB to load. The
  Watcom build needs about 17 KB.
- Smaller C's startup hooks interrupts 0, 4 and 6 and Ctrl-C, and puts
  them back on exit.

Apart from the speed of the arithmetic, startup and run time showed
nothing unusual. The tests above also ran `dfsc.exe` with DOSBox-X set
to an 8086 (`cputype=8086`), where it worked normally.

## Building

    make smallerc            # dfsc.exe
    make test-smallerc       # unit-smallerc, then tests/run.sh on dfsc.exe
    make unit-smallerc       # host unit tests against smallerc/format.c
    make dostest-smallerc    # tests/run.sh on dfsc.exe only
    make test-rows           # tests/rows.c on both DOS builds (needs both)

Tools, as tested (Ubuntu 26.04):

| Tool | Version | Notes |
| --- | --- | --- |
| Smaller C | 1.0.2 (`smallerc` 1.0.2+dfsg-1build1, Ubuntu) | `smlrcc`, `smlrc`, `smlrpp`, `smlrl`; the Ubuntu package (25.04 and later) or Debian's, or upstream tag `v1.0.2+dos.win.b120a9c` |
| NASM | 3.01 | Smaller C's assembler, also wrapped by `nasm8086` |
| Smaller C library source | commit `b120a9c389146bc2049dfff5fa16673210c8a12a` | fetched by `smallerc/toolchain.sh`, checked against its SHA-256 |
| curl, tar, sha256sum, patch | any | to fetch and prepare the library source |
| Open Watcom | 2.0 beta (Oct 20 2026) | default build and `make test-rows` |
| DOSBox-X | 2026.01.02 | DOS tests |
| host C compiler | GCC 15.2 | unit tests and `make test-rows` |

Override the tools with `make SMLRCC=... SMLRC_TC=...` (`SMLRC_TC` is
where the 8086 library is built, default `smallerc/toolchain`). The
first build downloads the library source. After that the archive is
kept and checked again on each rebuild.

Flags:

| Compiler | Flags |
| --- | --- |
| Watcom | `wcl -q -bt=dos -ms -0 -os -s -wx -we -zq` |
| Smaller C | `smlrcc -doss -Wall -asm smallerc/nasm8086 -I src` with `-SI`/`-SL` for the 8086 library and `smallerc/` |

## What it took

Apart from `src/format.h` (see [64-bit arithmetic](#64-bit-arithmetic)),
`src/` builds with Smaller C unchanged. The rest is in `smallerc/`:

- **`format.c`**: replaces `src/format.c`, as above.
- **`i86.h`, `i86.c`**: the part of Watcom's `<i86.h>` that
  `src/drives.c` uses: `union REGS` and `int86`. Smaller C has no
  `<i86.h>`, so `-SI smallerc` provides this one. `int86` is inline
  `asm()`. It calls the handler through the interrupt vector, because
  `int` takes its number from the instruction.
- **`8086.inc`, `nasm8086`**: Smaller C's 16-bit code generator emits
  some 80186 and 80386 instructions (`movzx`, `movsx`, `setcc`, `push
  imm`, `leave`, `imul reg, r/m, imm`, shifts by a count other than
  1). `8086.inc` is a set of NASM macros that replace them with 8086
  code and sets `cpu 8086`, so NASM rejects anything left over.
  `nasm8086` runs NASM with it pre-included and is passed to `smlrcc
  -asm`. These come from dos-which16. The one change is that `push
  imm` now also accepts `push byte N` and `push word N`, which Smaller
  C uses to pass structures by value.
- **`toolchain.sh`, `lcds-8086.patch`**: the `lcds.a` that Smaller C
  ships needs a 386 (its documentation says so). The script rebuilds
  it from the matching upstream source with `nasm8086`. The patch
  ports the only two library functions written in 386 assembly,
  `rand()` and `system()`. df uses neither, but the patch keeps the
  whole library 8086-clean. Both are unchanged from dos-which16.

Smaller C limitations found:

- no `long` or `long long` in 16-bit code (above);
- an integer constant above 32767 without a `u` suffix is an error
  ("Constant too big for 16-bit signed type"), because its type would
  be `long`. `src/` has none. `tests/rows.c` uses `32768u`;
- no `<i86.h>` or `<dos.h>`.

## Not changed

The default `make`, `make test`, CI and the release workflow still use
Watcom only, and `VERSION` is not changed. Watcom's `df.exe` is
byte-for-byte the same as before. CI does not build the Smaller C
variant: `ubuntu-24.04` has no `smallerc` package, and building
Smaller C from source in CI is left for a follow-up if this is
pursued.
