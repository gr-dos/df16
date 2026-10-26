#include <dos.h>
#include "drives.h"

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

    r.h.ah = 0x36;
    r.h.dl = (unsigned char)(drive + 1);
    int86(0x21, &r, &r);
    if (r.x.ax == 0xffff)
        return -1;
    df->spc = r.x.ax;
    df->avail = r.x.bx;
    df->bps = r.x.cx;
    df->total = r.x.dx;
    return 0;
}
