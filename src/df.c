#include "df.h"

static char line[FMT_LINESIZE];

struct report {
    const struct options *o;
    FILE *out;
    int rows;
};

static void put_row(struct report *r, int drive, const struct diskfree *df)
{
    struct dfsizes s;

    if (r->rows++ == 0) {
        fmt_header(line, r->o->human);
        fprintf(r->out, "%s\n", line);
    }
    df_sizes(df, &s);
    fmt_row(line, drive, &s, r->o->human);
    fprintf(r->out, "%s\n", line);
}

int df_run(const struct options *o, int argc, char **argv,
           const struct df_ops *ops, FILE *out, FILE *err)
{
    struct report r;
    struct diskfree df;
    int i, drive, last, operands = 0, current = -1;
    int rc = RC_OK;

    r.o = o;
    r.out = out;
    r.rows = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i] == NULL)
            continue;
        operands++;
        drive = operand_drive(argv[i]);
        if (drive == DRIVE_CURRENT) {
            if (current < 0)
                current = ops->current_drive();
            drive = current;
        }
        if (ops->query(drive, &df) != 0) {
            fprintf(err, "df: %s: drive %c: is invalid or not ready\n",
                    argv[i], 'A' + drive);
            rc = RC_FAIL;
            continue;
        }
        put_row(&r, drive, &df);
    }

    if (operands == 0) {
        last = ops->drive_count();
        if (last > 26)
            last = 26;
        /* Never query A: or B: without -a: probing an empty floppy
         * drive is slow and noisy. */
        for (drive = SCAN_FIRST(o); drive < last; drive++)
            if (ops->query(drive, &df) == 0)
                put_row(&r, drive, &df);
        if (r.rows == 0) {
            fprintf(err, "df: no drives found\n");
            rc = RC_FAIL;
        }
    }

    if (fflush(out) != 0 || ferror(out)) {
        fprintf(err, "df: write error\n");
        rc = RC_FATAL;
    }
    return rc;
}
