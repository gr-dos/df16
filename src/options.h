#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>

/* Command-line parsing.  Portable: no DOS calls. */

struct options {
    int all;            /* -a: include A: and B: in the default scan */
    int human;          /* -h; -k clears it; the last one wins */
};

/* parse_options results */
#define OPT_RUN      0
#define OPT_HELP     1
#define OPT_VERSION  2
#define OPT_USAGE    3      /* a diagnostic has been written to err */

/*
 * Parse argv into o.  Options (and "--") are removed by setting their
 * argv entries to NULL, leaving the operands in order.  Operands are
 * checked with operand_drive, so a malformed one is a usage error
 * before any drive is touched.
 */
int parse_options(int argc, char **argv, struct options *o, FILE *err);

/* operand_drive results other than a drive index */
#define DRIVE_CURRENT  (-1)     /* no drive prefix: the current drive */
#define DRIVE_BAD      (-2)     /* malformed */

/* The drive an operand names: 0 for A: ... 25 for Z:, or one of the
 * above.  "C:", "c:\FOO" and "C:FOO" are drive C; ".", "FOO" and
 * "\FOO" are the current drive; "1:", "" and "FOO:" are malformed. */
int operand_drive(const char *arg);

/* The first drive index of the default scan. */
#define SCAN_FIRST(o)  ((o)->all ? 0 : 2)

#endif
