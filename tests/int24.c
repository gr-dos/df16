/*
 * DOS test of df's critical-error handling (src/drives.c), run by
 * tests/run.sh under DOSBox-X.  DOSBox-X never raises a critical error
 * for its drives, so this checks the parts that can be checked there:
 * the handler answers Fail and preserves the other registers, and
 * dos_query leaves INT 24h as it found it, whether the drive is valid
 * or not.  Prints one line per failed check and "ok N" at the end;
 * ERRORLEVEL is the number of failures.
 */

#include <stdio.h>
#include "drives.h"

/* INT 24h with AX and BX given; returns AX and stores BX in *bx. */
static unsigned raise24(unsigned ax, unsigned bx, unsigned *bxout);
#pragma aux raise24 =    \
    "push si"            \
    "mov si,cx"          \
    "int 24h"            \
    "mov [si],bx"        \
    "pop si"             \
    __parm [__ax] [__bx] [__cx] __value [__ax] __modify [__bx];

static int checks, failures;

static void check(int ok, const char *what)
{
    checks++;
    if (!ok) {
        failures++;
        fputs("FAIL: ", stdout);
        fputs(what, stdout);
        fputs("\n", stdout);
    }
}

int main(void)
{
    dos_isr before = dos_get_vector(0x24);
    struct diskfree df;
    unsigned bx;
    char n[2];

    check(dos_query(2, &df) == 0, "C: is queried");
    check(dos_get_vector(0x24) == before, "INT 24h restored after C:");
    check(dos_query(24, &df) == -1, "Y: is invalid");
    check(dos_get_vector(0x24) == before, "INT 24h restored after Y:");

    dos_set_vector(0x24, dos_fail_handler);
    check(raise24(0x1a00, 0x1234, &bx) == 0x1a03, "handler returns AL=3");
    check(raise24(0x8001, 0x5678, &bx) == 0x8003, "handler keeps AH");
    check(bx == 0x5678, "handler keeps BX");
    dos_set_vector(0x24, before);
    check(dos_get_vector(0x24) == before, "INT 24h set back by the test");

    n[0] = (char)('0' + checks);
    n[1] = '\0';
    if (failures == 0) {
        fputs("ok ", stdout);
        fputs(n, stdout);
        fputs("\n", stdout);
    }
    return failures;
}
