/*
 * The part of Open Watcom's <i86.h> that src/drives.c uses, for
 * Smaller C's small model.  Smaller C has no <i86.h>; i86.c
 * implements int86.
 */

#ifndef SMALLERC_I86_H
#define SMALLERC_I86_H

struct WORDREGS {
    unsigned short  ax, bx, cx, dx, si, di;
    unsigned short  cflag;      /* nonzero if the carry flag was set */
};

struct BYTEREGS {
    unsigned char   al, ah, bl, bh, cl, ch, dl, dh;
};

union REGS {
    struct WORDREGS x;
    struct BYTEREGS h;
};

/* Call interrupt INTNO with the registers in IN; the registers after
 * it go to OUT (which may be IN).  Returns AX. */
int int86(int intno, union REGS *in, union REGS *out);

#endif
