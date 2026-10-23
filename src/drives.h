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
 * invalid (AX=FFFFh). */
int dos_query(int drive, struct diskfree *df);

#endif
