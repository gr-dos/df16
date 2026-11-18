#ifndef DRIVES_H
#define DRIVES_H

#include "format.h"

/* DOS drive calls (INT 21h).  Drives are 0 for A:, 1 for B:, ... */

/* AH=19h: the current drive. */
int dos_current_drive(void);

/* AH=0Eh with the current drive, so the default drive is unchanged:
 * the logical-drive count.  An upper bound; letters in range may be
 * unused. */
int dos_drive_count(void);

/* AH=36h: free-space query.  0 on success, -1 if DOS reports the drive
 * invalid or not ready (AX=FFFFh).  On DOS 3.0 and later, a critical
 * error during the query is answered Fail instead of prompting (see
 * drives.c). */
int dos_query(int drive, struct diskfree *df);

/* An interrupt handler, as INT 21h AH=35h and AH=25h see it. */
typedef void (__interrupt __far *dos_isr)();

/* AH=35h: the vector for interrupt n. */
dos_isr dos_get_vector(unsigned char n);
#pragma aux dos_get_vector = \
    "mov ah,35h"             \
    "int 21h"                \
    "mov dx,es"              \
    "mov ax,bx"              \
    __parm [__al] __value [__dx __ax] __modify [__bx __es];

/* AH=25h: set the vector for interrupt n. */
void dos_set_vector(unsigned char n, dos_isr handler);
#pragma aux dos_set_vector = \
    "push ds"                \
    "mov ds,cx"              \
    "mov ah,25h"             \
    "int 21h"                \
    "pop ds"                 \
    __parm [__al] [__cx __dx] __modify [__ah];

/* The INT 24h (critical error) handler dos_query installs: it answers
 * Fail (AL=3) and changes no other register. */
void __interrupt __far dos_fail_handler();

#endif
