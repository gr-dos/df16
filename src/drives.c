#include <dos.h>
#include "drives.h"

/*
 * Critical errors (INT 24h).  A query of a removable or otherwise
 * unready drive can make DOS raise a critical error, whose default
 * handler asks "Abort, Retry, Fail?" and waits for a key.  For the
 * length of each free-space query, df installs a handler that answers
 * Fail, so the query returns AX=FFFFh and the drive is reported as not
 * ready (or skipped by the default scan).  The previous handler is put
 * back as soon as the query returns, so critical errors anywhere else,
 * such as writing the report to a floppy, still prompt as usual.  If df
 * is ended during a query, DOS restores INT 24h from the PSP itself.
 *
 * Fail (AL=3) is DOS 3.0 and later; on DOS 2.x the handler is not
 * installed and the query behaves as it always has.
 */

#define CRITICAL_ERROR 0x24
#define FAIL           3

/* The registers an Open Watcom __interrupt function saves, as its
 * argument sees them; the values in it are restored on return. */
struct isr_regs {
    unsigned gs, fs, es, ds, di, si, bp, sp, bx, dx, cx, ax, ip, cs, flags;
};

/* Answer Fail.  Calls nothing: DOS is not reentrant here. */
void __interrupt __far dos_fail_handler(struct isr_regs r)
{
    r.ax = (r.ax & 0xff00) | FAIL;
}

static int dos_major(void)
{
    static int major = -1;
    union REGS r;

    if (major < 0) {
        r.x.ax = 0x3000;
        int86(0x21, &r, &r);
        major = r.h.al;
    }
    return major;
}

int dos_current_drive(void)
{
    union REGS r;

    r.h.ah = 0x19;
    int86(0x21, &r, &r);
    return r.h.al;
}

int dos_drive_count(void)
{
    union REGS r;

    r.h.ah = 0x0e;
    r.h.dl = (unsigned char)dos_current_drive();
    int86(0x21, &r, &r);
    return r.h.al;
}

int dos_query(int drive, struct diskfree *df)
{
    union REGS r;
    dos_isr saved = 0;
    int fail = dos_major() >= 3;

    if (fail) {
        saved = dos_get_vector(CRITICAL_ERROR);
        dos_set_vector(CRITICAL_ERROR, dos_fail_handler);
    }
    r.h.ah = 0x36;
    r.h.dl = (unsigned char)(drive + 1);
    int86(0x21, &r, &r);
    if (fail)
        dos_set_vector(CRITICAL_ERROR, saved);
    if (r.x.ax == 0xffff)
        return -1;
    df->spc = r.x.ax;
    df->avail = r.x.bx;
    df->bps = r.x.cx;
    df->total = r.x.dx;
    return 0;
}
