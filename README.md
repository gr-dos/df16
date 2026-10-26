# df16

A Unix `df`-style disk space report for 16-bit MS-DOS, built with Open
Watcom and [libc16](https://github.com/gr-dos/libc16). It shows each drive's total, used and available space and the
percentage used.

A plain `df` **never touches A: or B:**, so it does not spin up or
wait on empty floppy drives. Name a floppy drive (`df a:`) or use `-a`
to include it.

The resulting `df.exe` is a small-model, real-mode 8086 executable
(about 5 KB) that runs on any DOS from an original PC up.

## Building

`df.exe` links [libc16](https://github.com/gr-dos/libc16), a C runtime
for small-model DOS programs, instead of Open Watcom's own, and is
built with the Open Watcom toolchain that libc16 pins. By default
`make` runs `scripts/fetch-libc16`, which clones libc16 at the revision
pinned in that script into `build/libc16` and builds it there; libc16's
build fetches its Open Watcom snapshot and JWasm into
`build/libc16/build/tools`. This needs `git`, `curl`, `xz` and a host C
compiler. To move to a newer libc16, change the pin.

    make                         # fetches libc16 if needed, builds df.exe
    make LIBC16_ROOT=../libc16   # or use a built local libc16 checkout
    make test                    # host unit tests, then DOS tests
    make unit                    # host unit tests only
    make dostest                 # tests/run.sh under DOSBox-X only
    make clean

The unit tests need a C99 host compiler (`HOSTCC`, default `cc`). The
DOS tests need `dosbox-x` on the host (override with `DOSBOX=`); set
`KEEP=1` to keep the work directory.

The version is the one line in `VERSION`, DOS style (0.01, 0.02, ...
1.00), not semantic versioning. When a PR that changes it is merged to
`main`, the release workflow tags the merge commit `v<VERSION>` and
publishes a GitHub release with `df.exe` attached.

## Usage

    df [OPTION]... [DRIVE-OR-PATH]...

Options are case-insensitive and short ones may be grouped (`-ah`).

| Option | Meaning |
| --- | --- |
| `-a` | include A: and B: in the default scan |
| `-h` | human-readable sizes (KiB, MiB, GiB) |
| `-k` | sizes in 1 KiB blocks (the default) |
| `-?`, `--help` | show help |
| `--version` | show version |
| `--` | treat the remaining arguments as operands |

`-h` always means human-readable, never help. Of `-h` and `-k`, the last
one given wins.

With no operands, `df` reports every drive from C: up to DOS's last
drive (from A: with `-a`), skipping letters that are not in use. With
operands, it reports exactly the drives they name, in order, one row
each, and A: or B: is queried if named.

An operand selects a drive: `C:`, `C:\FOO\BAR` and `C:FOO` mean C:, and
`.` or any path without a drive means the current drive. The rest of
the path is **not** checked: `df C:\NOTHERE` reports C:. Drive letters
must be A to Z; `1:` is an error.

### Examples

    C:\> df
    Drive      1K-blocks       Used      Avail  Use%
    C:            124906      16076     108830   13%
    D:            524288     131072     393216   25%
    C:\> df -h c:
    Drive           Size       Used      Avail  Use%
    C:          122.0MiB    15.7MiB   106.3MiB   13%
    C:\> df a:
    Drive      1K-blocks       Used      Avail  Use%
    A:              1423        512        911   36%

(The sizes above are illustrative.)

### Output

One header line, then one line per drive, in drive-letter order for the
default scan or operand order otherwise. Lines end in CR LF and fit an
80-column screen.

- 1 KiB blocks are rounded up, so a partly used block counts.
- `-h` sizes are binary (1 KiB = 1024 bytes): whole bytes below 1 KiB
  (`512B`), otherwise one decimal place, rounded up, so a nonzero size
  never shows as zero.
- Use% is rounded up. A drive reporting no capacity shows `-`.
- If DOS reports more free clusters than total clusters, free is taken
  to be the total.

### ERRORLEVEL

| ERRORLEVEL | Meaning |
| --- | --- |
| 0 | every named drive was reported, or the scan found at least one drive |
| 1 | a named drive was invalid or not ready, or the scan found no drives |
| 2 | usage error, or a write error on standard output |

A drive that fails is reported on standard error and the remaining
operands are still processed.

## Limits

- Free space comes from the classic DOS call (INT 21h AH=36h). Its
  16-bit cluster counts cannot describe volumes much beyond 2 GB, so
  on large volumes or later DOS versions the sizes may be wrong.
- A: and B: are skipped by convention, not by detecting floppy drives.
  A floppy or other removable drive at C: or later is still queried by
  a plain `df`, and an empty one may be slow or prompt
  `Abort, Retry, Fail?`.
- No labels, filesystem types or per-directory usage.

## Files

- `src/main.c`: the command-line program
- `src/options.c`, `src/options.h`: option and operand parsing
- `src/df.c`, `src/df.h`: drive selection and the report
- `src/format.c`, `src/format.h`: capacity arithmetic and formatting
- `src/drives.c`, `src/drives.h`: the DOS drive calls
- `src/out.c`, `src/out.h`: writing strings without `printf`
- `tests/test_*.c`: host unit tests (DOS calls mocked in `test_df.c`)
- `tests/run.sh`: DOS tests, run under DOSBox-X
- `docs/DESIGN.md`: the design
- `Makefile`: Open Watcom and libc16 build
- `scripts/fetch-libc16`: fetches and builds the pinned libc16
- `VERSION`: the version, compiled in and printed by `--version`

## License

MIT. See `LICENSE`.
