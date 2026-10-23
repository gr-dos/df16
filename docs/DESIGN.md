# dos-df16 — Design

## 1. Purpose

`dos-df16` is a small, 8086-compatible MS-DOS utility that reports filesystem capacity and free space in a familiar Unix `df`-style format. It is written in C and built with Open Watcom, using `dos-hello16` as the project/build/release template.

The primary target is MS-DOS 6.22 on real XT-class hardware (including the NuXT/NEC V20). It should also work in DOSBox-X and DOSEMU2.

**Key requirement:** An ordinary `df` invocation must not access floppy drives, because probing empty floppy drives can be slow and noisy. Explicitly requested floppy drives may be accessed.

## 2. Scope

### MVP

- Enumerate logical drive letters, then report available drives other than A: and B: by default.
- Report total, used, and available capacity and percentage used.
- Support explicit drive and path operands, including `.`, `C:`, `C:\\FOO\\BAR`, and `D:FOO`.
- Support `-a`, `-h`, `-k`, `-?`, `--help`, and `--version`.
- Case-insensitive option names, consistent with other `dos-*16` tools.
- 8086-compatible 16-bit DOS executable built with Open Watcom.
- Automated host-side tests for pure formatting/parsing logic and DOS emulator integration tests.

### Out of scope for MVP

- Disk labels, filesystem types, physical-device identity, partition layout, and mount points.
- Accurate capacity reporting for filesystems that exceed the limits of the MS-DOS 6.22 free-space API.
- Distinguishing every possible removable device or network redirector from a fixed disk.
- Probing physical media to determine whether a drive is a floppy.
- Directory-level space accounting (all directories on a drive report the same volume capacity).

## 3. Command-line interface

```text
Usage: df [OPTION]... [DRIVE-OR-PATH]...
Report disk space for DOS drives.

  -a           include A: and B: in the default scan
  -h           human-readable sizes (KiB, MiB, GiB)
  -k           sizes in 1 KiB units (default)
  -?           display help and exit
  --help       display help and exit
  --version    display program version and exit
  --           end option processing
```

Option parsing is case-insensitive (`-H`, `--VERSION`, etc.). `-h` **always** means human-readable; it is **not** a help alias. `-?` and `--help` provide help.

`-h` and `-k` are mutually overriding formatting choices; the last one supplied wins. Short options may be grouped (for example, `-ah`). Unknown options and missing/invalid operands are usage errors.

Examples:

```dos
DF
DF -H
DF -A
DF -AH
DF C:
DF A:
DF .
DF C:\\FOO\\BAR
DF D:FOO
DF -H C: D:
DF --HELP
DF --VERSION
```

### Operand semantics

- With **no operands**, scan the DOS logical-drive range. Skip A: and B: entirely unless `-a` is specified.
- With **one or more operands**, report only the drives indicated by those operands. An explicitly requested `A:` or `B:` is always queried, regardless of `-a`.
- `.` and other paths without a drive prefix refer to the current DOS drive.
- `X:` and `X:\\...` refer to drive X. `X:relative` also refers to X, irrespective of X's current directory.
- Repeated operands may produce repeated rows, matching the request order. Do not silently expand a path to multiple drives.
- A path operand is used to select its **drive**. The MVP does not require traversing or validating the full directory path: `DF C:\\MISSING` may still report C:'s capacity. Document this behavior rather than implying the path was checked.
- Drive letters must be ASCII A–Z; reject malformed drive prefixes such as `1:`. Treat a leading `-` as an option unless it follows `--`.

## 4. Drive discovery and floppy policy

### 4.1 Discover logical-drive upper bound

Use DOS `INT 21h`:

1. `AH=19h` — get current drive (AL: 0=A, 1=B, ...).
2. `AH=0Eh`, `DL=<current drive>` — select that same drive and obtain in AL the DOS logical-drive count / upper bound.

The returned count is an **upper bound**, not proof that every drive in the range exists. `LASTDRIVE` and redirectors can reserve unused letters. Clamp iteration to the 26 possible drive letters.

Because `AH=0Eh` selects a drive, pass the current drive back to avoid intentionally changing the user's default drive.

### 4.2 Default scan

For drive indexes `2` through `min(count, 26)-1` (C: onward), attempt the free-space query. Skip nonexistent/unsupported drives without printing a row; see error handling below.

**Never call the free-space API on A: or B: during the default scan.** Even an apparently harmless probe can spin up or seek a floppy drive.

### 4.3 `-a` and explicit operands

With `-a` and no operands, scan A: through the upper bound, accepting that empty floppy drives may prompt, fail, or take time.

For explicitly requested drives, query exactly those drives, including A: or B:. Do not perform a preliminary media probe that would duplicate accesses.

### 4.4 Limitation

A: and B: are conventionally floppy letters, not an infallible hardware classification. A floppy-like or removable device mapped to C: or later may still be slow to query. The MVP deliberately uses the predictable DOS drive-letter convention rather than complex device-type detection.

## 5. Disk-space API

Use `INT 21h, AH=36h` (**Get Free Disk Space**):

- Input: `DL=1` for A:, `2` for B:, `3` for C:, etc.; `DL=0` means the current drive.
- Output on success:
  - `AX`: sectors per cluster
  - `BX`: free clusters
  - `CX`: bytes per sector
  - `DX`: total clusters
- `AX=FFFFh`: invalid drive / unsupported query.

The byte counts are:

```text
cluster_bytes = sectors_per_cluster * bytes_per_sector
total_bytes   = total_clusters * cluster_bytes
free_bytes    = free_clusters * cluster_bytes
used_bytes    = total_bytes - free_bytes
use_percent   = ceil(100 * used_bytes / total_bytes)
```

For `total_bytes == 0`, display a placeholder percentage instead of dividing by zero. Clamp anomalous results (such as free clusters greater than total clusters) or report them as errors rather than underflowing.

**Arithmetic:** Do not multiply all three 16-bit values in a 16-bit or 32-bit intermediate. Use a sufficiently wide integer representation (prefer a verified 64-bit type supported by the chosen Open Watcom target) or carefully staged arithmetic. Verify actual compiler support and generated 8086 compatibility. Avoid floating-point dependencies.

MS-DOS 6.22's classic API has known large-volume limits; the utility should document that results on later DOS variants or very large volumes may be inaccurate. Extended DOS 7+ APIs are a potential future enhancement, not a prerequisite.

## 6. Output

Suggested default format (KiB units):

```text
Drive      1K-blocks       Used      Avail  Use%
C:            124906      16077     108829   13%
D:            524288     131072     393216   25%
```

Suggested `-h` format:

```text
Drive          Size       Used      Avail  Use%
C:          122.0MiB    15.7MiB   106.3MiB   13%
D:          512.0MiB   128.0MiB   384.0MiB   25%
```

Formatting rules:

- Default and `-k`: report 1 KiB blocks, using **ceiling division** when converting bytes to blocks, so nonzero partial KiB values are not silently discarded. The exact rounding policy should be covered by tests.
- `-h`: binary multiples of 1024 with suffixes `B`, `KiB`, `MiB`, `GiB` (and `TiB` if representable); choose a unit appropriate to the value. Use consistent precision and avoid displaying a positive value as zero.
- Percentage used: round **up** to the nearest integer, as is common for Unix `df`.
- Display one line per reported drive, in drive-letter order for default enumeration or operand order when operands are supplied.
- Align columns for readability on an 80-column DOS screen. Use CRLF line endings under DOS.
- Keep output stable and simple enough for batch files and test scripts.

## 7. Error handling and exit status

Suggested `ERRORLEVEL` convention, aligned with `dos-crcsum16`:

| Code | Meaning |
| --- | --- |
| `0` | All requested queries succeeded; or default enumeration completed with at least one reported drive. |
| `1` | One or more explicitly requested drives could not be queried, or no usable drives were found. |
| `2` | Invalid command-line syntax or output write failure. |

For the default scan, quietly skip unavailable letters because gaps in the DOS drive range are normal. If a real drive is present but cannot provide free-space information, a future verbose option may distinguish that from a nonexistent drive; the MVP need not guess.

For explicit operands, print a diagnostic on stderr and continue processing remaining operands. Return the worst encountered exit status.

Check output errors (including a failed final `fflush(stdout)`) and return 2 if reporting fails.

`--help` and `--version` print to stdout and return 0 without probing drives.

## 8. Architecture

Keep DOS-specific calls isolated from portable logic:

- `src/main.c`: command-line parsing, drive selection, output, exit codes.
- `src/drives.c` / `.h`: current-drive lookup, drive upper bound, free-space query via DOS interrupt.
- `src/format.c` / `.h`: capacity formatting, percent calculation, column layout.

The precise file split may be adjusted to keep the implementation small. Avoid introducing a framework or dynamic allocation without need.

Use the `dos-hello16` project/template conventions for `VERSION`, Open Watcom build flags, CI, README, licensing, and release packaging. Version display should follow that template's actual format rather than inventing a conflicting convention.

## 9. Testing

### 9.1 Host unit tests

Test portable parsing and formatting independently of DOS:

- `-h` vs `--help` vs `-?`, including case-insensitive spelling.
- Grouped options, `--` termination, unknown options, `-h`/`-k` precedence.
- Drive/path selection: `.`, `C:`, `C:\\FOO\\BAR`, `D:FOO`, lowercase drive letters, malformed prefixes.
- Capacity arithmetic near 16-bit and 32-bit boundaries, zero capacity, invalid cluster counts.
- Human-readable unit transitions and percentage rounding.
- Default scan starts at drive index 2; `-a` starts at index 0.

### 9.2 DOSBox-X / DOSEMU2 integration tests

- Build and execute the 16-bit DOS binary.
- Verify `--help`/`--version` do not access drives.
- Verify `DF`, `DF C:`, `DF .`, `DF -H C:` and error levels.
- Verify that a missing letter is skipped in default enumeration and reported as an error when explicitly requested.
- Capture output to files and compare with expected patterns; do not hard-code emulator-dependent disk capacities.
- Where possible, instrument or mock the drive-query boundary to verify that the default scan never queries A: or B:.

### 9.3 Real hardware: Phaistos

- Run `DF` with no floppy inserted; confirm **no floppy motor activity or delay**.
- Run `DF C:` and compare with `CHKDSK C:` or another trusted DOS capacity report.
- Run `DF .` and `DF C:\\SOME\\PATH`; confirm correct drive selection.
- Run `DF -H` and inspect output on an 80-column display.
- Optionally run `DF A:` with a known-good floppy to validate explicit floppy support.
- Confirm clean execution on the NuXT NEC V20 and MS-DOS 6.22.

## 10. Acceptance criteria

The MVP is ready when:

1. A working 8086-compatible `DF.EXE` builds with Open Watcom.
2. Bare `DF` lists available drives from C: onward without querying A: or B:.
3. `DF -A` includes A: and B:, and `DF A:` explicitly queries A:.
4. `DF .`, `DF C:`, and `DF C:\\FOO\\BAR` select the expected drive.
5. `-h`, `-k`, `-?`, `--help`, `--version`, and case-insensitive variants behave as documented.
6. Capacity calculations and exit codes pass automated tests.
7. Real-hardware smoke testing on Phaistos confirms correct reporting and no default floppy access.

## 11. Possible future work

- Optional filesystem/volume labels and filesystem type.
- Explicit filtering of network, optical, or removable drives when reliably detectable.
- Extended free-space APIs on DOS 7+.
- A machine-readable or terse output mode.
- More advanced path validation if needed.
