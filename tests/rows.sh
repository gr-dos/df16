#!/bin/sh
#
# Check that DOS builds of tests/rows.c print the same rows as the host
# build.  Each DOS program runs under DOSBox-X.
#
# Usage: tests/rows.sh HOST-PROGRAM DOS-EXE...
# Environment: DOSBOX (default dosbox-x), KEEP=1 to keep the work
#              directory.

set -u

DOSBOX=${DOSBOX:-dosbox-x}
command -v "$DOSBOX" >/dev/null || { echo "$DOSBOX not found" >&2; exit 2; }
[ $# -ge 2 ] || { echo "usage: $0 HOST-PROGRAM DOS-EXE..." >&2; exit 2; }

host=$1
shift
work=$(mktemp -d "${TMPDIR:-/tmp}/dosrows.XXXXXX")
fail=0

"$host" > "$work/want.txt" || { echo "$host failed" >&2; exit 2; }

for exe in "$@"; do
    rm -rf "$work/c"
    mkdir "$work/c"
    cp "$exe" "$work/c/rows.exe"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 600 \
        "$DOSBOX" -defaultconf -silent -nogui -nomenu \
        -set "cpu cycles=max" \
        -c "mount c \"$work/c\"" -c "c:" \
        -c "rows >out.txt" -c "exit" >"$work/dosbox.log" 2>&1
    tr -d '\r' < "$work/c/OUT.TXT" > "$work/got.txt" 2>/dev/null
    if cmp -s "$work/want.txt" "$work/got.txt"; then
        echo "ok: $exe ($(wc -l < "$work/want.txt") lines)"
    else
        echo "FAIL: $exe differs from $host"
        diff "$work/want.txt" "$work/got.txt" | head -20 | sed 's/^/    /'
        fail=1
    fi
done

if [ "$fail" -ne 0 ] || [ "${KEEP:-0}" = 1 ]; then
    echo "work directory: $work"
else
    rm -rf "$work"
fi
[ "$fail" -eq 0 ]
