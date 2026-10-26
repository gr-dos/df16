/*
 * Open Watcom's int86 for Smaller C's small model (DS = ES = SS, near
 * pointers).  See i86.h.
 *
 * Smaller C passes arguments on the stack, the first at [bp + 4], and
 * returns in ax.  Only bp and sp need to be preserved.
 */

#include <i86.h>

/*
 * INT takes its number from the instruction, so call the handler
 * through the vector instead: push an interrupt frame (flags, then the
 * return address) and jump to the handler with retf, so that its iret
 * comes back here.  ES is 0 only while reading the vector.
 */
int int86(int intno, union REGS *in, union REGS *out)
{
    asm("push bp\n"
        "xor  ax, ax\n"
        "mov  es, ax\n"
        "mov  bl, [bp + 4]\n"
        "xor  bh, bh\n"
        "shl  bx, 1\n"
        "shl  bx, 1\n"
        "pushf\n"
        "push cs\n"
        "mov  ax, ___int86_back\n"
        "push ax\n"
        "push word [es:bx + 2]\n"
        "push word [es:bx]\n"
        "push ds\n"
        "pop  es\n"
        "mov  bx, [bp + 6]\n"
        "mov  ax, [bx]\n"
        "mov  cx, [bx + 4]\n"
        "mov  dx, [bx + 6]\n"
        "mov  si, [bx + 8]\n"
        "mov  di, [bx + 10]\n"
        "mov  bx, [bx + 2]\n"
        "cli\n"
        "retf\n"
        "___int86_back:\n"
        "pop  bp\n"
        "pushf\n"
        "push bx\n"
        "push ss\n"
        "pop  ds\n"
        "push ss\n"
        "pop  es\n"
        "mov  bx, [bp + 8]\n"
        "mov  [bx], ax\n"
        "pop  word [bx + 2]\n"
        "mov  [bx + 4], cx\n"
        "mov  [bx + 6], dx\n"
        "mov  [bx + 8], si\n"
        "mov  [bx + 10], di\n"
        "pop  ax\n"
        "and  ax, 1\n"
        "mov  [bx + 12], ax\n"
        "mov  ax, [bx]");
}
