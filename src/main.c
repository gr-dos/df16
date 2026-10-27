/*
 * df - report disk space for DOS drives
 *
 * A Unix df-style free-space report for 16-bit MS-DOS (Open Watcom C
 * with libc16).
 * A plain "df" never touches A: or B:.
 */

#include <stdio.h>
#include "df.h"
#include "drives.h"
#include "options.h"

#ifndef VERSION
#define VERSION   "unknown"
#endif

static const struct df_ops dos_ops = {
    dos_current_drive,
    dos_drive_count,
    dos_query
};

static void usage(void)
{
    fputs(
        "Usage: df [OPTION]... [DRIVE-OR-PATH]...\n"
        "Report disk space for DOS drives.\n"
        "\n"
        "  -a           include A: and B: in the default scan\n"
        "  -h           human-readable sizes (KiB, MiB, GiB)\n"
        "  -k           sizes in 1 KiB units (default)\n"
        "  -?           display help and exit\n"
        "  --help       display help and exit\n"
        "  --version    display program version and exit\n"
        "  --           end option processing\n"
        "\n"
        "With no operands, report drives C: onward (A: onward with -a).\n"
        "A path selects its drive only; the path itself is not checked.\n"
        "\n"
        "ERRORLEVEL is 0 on success, 1 if a requested drive could not be\n"
        "read or no drives were found, and 2 for a usage or write error.\n",
        stdout);
}

static int finish(void)
{
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fputs("df: write error\n", stderr);
        return RC_FATAL;
    }
    return RC_OK;
}

int main(int argc, char **argv)
{
    struct options o;

    switch (parse_options(argc, argv, &o, stderr)) {
    case OPT_HELP:
        usage();
        return finish();
    case OPT_VERSION:
        fputs("df (dos-df16) " VERSION "\n", stdout);
        return finish();
    case OPT_USAGE:
        fputs("Try 'df --help' for more information.\n", stderr);
        return RC_FATAL;
    }
    return df_run(&o, argc, argv, &dos_ops, stdout, stderr);
}
