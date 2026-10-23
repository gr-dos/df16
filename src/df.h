#ifndef DF_H
#define DF_H

#include <stdio.h>
#include "format.h"
#include "options.h"

/* exit statuses, also used as "worst so far" */
#define RC_OK     0
#define RC_FAIL   1     /* a requested drive failed, or no drives found */
#define RC_FATAL  2     /* usage error or output write error */

/*
 * The drive-query boundary.  main.c supplies the DOS calls; the host
 * tests supply mocks.  Drives are 0 for A:, 1 for B:, ...
 */
struct df_ops {
    int (*current_drive)(void);
    /* DOS's logical-drive count: an upper bound, not all present */
    int (*drive_count)(void);
    /* 0 on success, nonzero if the drive is invalid or not ready */
    int (*query)(int drive, struct diskfree *df);
};

/*
 * Report the drives named by the non-NULL argv[1..argc-1] (as left by
 * parse_options), or scan the drive range if there are none.  Rows go
 * to out and diagnostics to err.  Returns the exit status.
 */
int df_run(const struct options *o, int argc, char **argv,
           const struct df_ops *ops, FILE *out, FILE *err);

#endif
