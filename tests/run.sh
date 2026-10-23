#!/bin/sh
#
# Test df.exe under DOSBox-X.
#
# The work directory's c/ is mounted as C:, d/ as D: and a/ as the
# floppy drive A:.  One batch file runs every DOS-side step, and the
# results are checked on the host afterwards.  Each step N records df's
# exit status in RN.TXT and its standard output in ON.TXT.  DOS cannot
# redirect standard error, so diagnostics are not checked.  rc.bat uses
# goto rather than "if errorlevel N echo ... >file" because the DOSBox-X
# shell opens the redirection even when the test is false.
#
# Capacities depend on the emulator, so rows are checked against a
# pattern and for consistency, not for exact numbers.  That the default
# scan never even queries A: or B: is checked by the host unit tests
# (tests/test_df.c); here it is checked that A: is not reported.
#
# To add a test: add a step to steps.bat (command, then "call rc N")
# and the matching rc/expect/check lines below.
#
# Usage: tests/run.sh [path/to/df.exe]
# Environment: DOSBOX (default dosbox-x), KEEP=1 to keep the work
#              directory.

set -u

top=$(cd "$(dirname "$0")/.." && pwd)
exe=${1:-$top/df.exe}
DOSBOX=${DOSBOX:-dosbox-x}

command -v "$DOSBOX" >/dev/null || { echo "$DOSBOX not found" >&2; exit 2; }
[ -f "$exe" ] || { echo "$exe not found" >&2; exit 2; }

work=$(mktemp -d "${TMPDIR:-/tmp}/dosdf.XXXXXX")
pass=0
fail=0
LC_ALL=C
export LC_ALL

# ------------------------------------------------------------------
# fixtures

cd "$work" || exit 2
mkdir c d a
cp "$exe" c/df.exe

cat > c/rc.bat <<'EOS'
@echo off
if errorlevel 2 goto rc2
if errorlevel 1 goto rc1
echo rc0 >c:\r%1.txt
goto end
:rc1
echo rc1 >c:\r%1.txt
goto end
:rc2
echo rc2 >c:\r%1.txt
:end
EOS

cat > c/steps.bat <<'EOS'
@echo off
rem --- default scan
df >o1.txt
call rc 1
df -a >o2.txt
call rc 2
df -A >o3.txt
call rc 3
df -h >o4.txt
call rc 4
df -H >o5.txt
call rc 5
df -ah >o6.txt
call rc 6
df -k >o7.txt
call rc 7
rem --- explicit drives and paths
df c: >o8.txt
call rc 8
df . >o9.txt
call rc 9
df C:\NOTHERE\BAR >o10.txt
call rc 10
df d:foo >o11.txt
call rc 11
df a: >o12.txt
call rc 12
df -h c: d: >o13.txt
call rc 13
df -k -h -k C: >o14.txt
call rc 14
df c: a: c: >o15.txt
call rc 15
df -- c: >o16.txt
call rc 16
d:
c:\df . >c:\o17.txt
call c:\rc 17
c:
rem --- missing drives
df y: >o18.txt
call rc 18
df c: y: d: >o19.txt
call rc 19
rem --- usage errors
df 1: >o20.txt
call rc 20
df c: 1: >o21.txt
call rc 21
df -x >o22.txt
call rc 22
df --bogus >o23.txt
call rc 23
df - >o24.txt
call rc 24
rem --- help and version
df --help >o25.txt
call rc 25
df -? >o26.txt
call rc 26
df --HELP >o27.txt
call rc 27
df --version >o28.txt
call rc 28
df --VERSION >o29.txt
call rc 29
EOS

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 \
    "$DOSBOX" -defaultconf -silent -nogui -nomenu \
    -set "cpu cycles=max" \
    -c "mount c \"$work/c\"" -c "mount d \"$work/d\"" \
    -c "mount a \"$work/a\" -t floppy" -c "c:" \
    -c "steps.bat" -c "exit" >dosbox.log 2>&1

cd c || exit 2

# ------------------------------------------------------------------
# checks

ok()     { pass=$((pass + 1)); }
not_ok() { echo "FAIL: $*"; fail=$((fail + 1)); }

# check NAME COMMAND...
check() {
    name=$1
    shift
    if "$@"; then ok; else not_ok "$name"; fi
}

# rc STEP EXPECTED-STATUS
rc() {
    got=$(tr -d ' \r' < "R$1.TXT" 2>/dev/null | sed 's/^rc//')
    if [ "$got" = "$2" ]; then
        ok
    else
        not_ok "step $1: exit status ${got:-missing}, expected $2"
    fi
}

# expect STEP < expected-lines; compares ON.TXT with CRs removed
expect() {
    tr -d '\r' < "O$1.TXT" 2>/dev/null > got.txt
    cat > want.txt
    if cmp -s got.txt want.txt; then
        ok
    else
        not_ok "step $1: output differs"
        diff want.txt got.txt | sed 's/^/    /'
    fi
}

# drives STEP: the drive letters ON.TXT reports, in order, on one line
drives() {
    tr -d '\r' < "O$1.TXT" 2>/dev/null | sed -n 's/^\([A-Z]\):.*/\1/p' |
        tr -d '\n'
}

# letters STEP EXPECTED: ON.TXT reports exactly these drives in order
letters() {
    got=$(drives "$1")
    if [ "$got" = "$2" ]; then
        ok
    else
        not_ok "step $1: drives '$got', expected '$2'"
    fi
}

# first STEP LETTERS: the report starts with these drives
first() {
    got=$(drives "$1")
    case $got in
    "$2"*) ok ;;
    *) not_ok "step $1: drives '$got', expected to start with '$2'" ;;
    esac
}

# without STEP LETTER: the report does not include LETTER
without() {
    case $(drives "$1") in
    *"$2"*) not_ok "step $1: reports $2:" ;;
    *) ok ;;
    esac
}

KHDR='Drive      1K-blocks       Used      Avail  Use%'
HHDR='Drive           Size       Used      Avail  Use%'
HSIZE='[0-9]+(B|\.[0-9](KiB|MiB|GiB|TiB))'

# rows STEP HUMAN: ON.TXT is the right header, then well-formed rows.
# In KiB, used + avail is total, or total + 1 from rounding both up.
rows() {
    tr -d '\r' < "O$1.TXT" 2>/dev/null > got.txt
    if [ "$2" = 1 ]; then hdr=$HHDR; else hdr=$KHDR; fi
    if [ "$(head -n 1 got.txt)" != "$hdr" ]; then
        not_ok "step $1: header"
        head -n 1 got.txt | sed 's/^/    /'
        return
    fi
    ok
    if [ "$2" = 1 ]; then
        bad=$(sed 1d got.txt | grep -Ev \
            "^[A-Z]: +$HSIZE +$HSIZE +$HSIZE +([0-9]+%|-)\$")
    else
        bad=$(sed 1d got.txt | awk '
            !/^[A-Z]: +[0-9]+ +[0-9]+ +[0-9]+ +([0-9]+%|-)$/ ||
            $3 + $4 < $2 || $3 + $4 > $2 + 1 { print }')
    fi
    if [ -n "$bad" ]; then
        not_ok "step $1: malformed rows"
        echo "$bad" | sed 's/^/    /'
    else
        ok
    fi
}

# same STEP OTHER: the two steps' output is identical
same() { check "step $1: same as step $2" cmp -s "O$1.TXT" "O$2.TXT"; }

# default scan: C: onward, A: only with -a
rc 1 0
rows 1 0
first 1 CD
without 1 A
without 1 B
check "step 1: lines end in CR LF" \
    sh -c "[ \$(grep -c \"\$(printf '\r')\$\" O1.TXT) -eq \$(wc -l < O1.TXT) ]"
rc 2 0
rows 2 0
first 2 ACD
rc 3 0
same 3 2
rc 4 0
rows 4 1
first 4 CD
without 4 A
rc 5 0
same 5 4
rc 6 0
rows 6 1
first 6 ACD
rc 7 0
same 7 1

# explicit drives and paths; the path itself is not checked
rc 8 0
rows 8 0
letters 8 C
for n in 9 10 14 16; do
    rc $n 0
    same $n 8
done
rc 11 0
rows 11 0
letters 11 D
rc 12 0
rows 12 0
letters 12 A
rc 13 0
rows 13 1
letters 13 CD
rc 15 0
rows 15 0
letters 15 CAC
# "." on D: is D:
rc 17 0
same 17 11

# missing drives: an error, but the rest are still reported
rc 18 1
expect 18 < /dev/null
rc 19 1
rows 19 0
letters 19 CD

# usage errors
for n in 20 21 22 23 24; do
    rc $n 2
    expect $n < /dev/null
done

# help and version
rc 25 0
check "step 25: help" grep -q '^Usage: df' O25.TXT
for n in 26 27; do
    rc $n 0
    same $n 25
done
for n in 28 29; do
    rc $n 0
    expect $n <<EOF
df (dos-df16) $(cat "$top/VERSION")
EOF
done

# ------------------------------------------------------------------

echo "$pass passed, $fail failed"
if [ "$fail" -ne 0 ] || [ "${KEEP:-0}" = 1 ]; then
    echo "work directory: $work"
else
    rm -rf "$work"
fi
[ "$fail" -eq 0 ]
