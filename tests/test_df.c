/* Host-side tests for df.c, with the DOS drive calls mocked. */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "df.h"
#include "unit.h"

/* mock DOS: a drive is present if present[drive] */
static int present[26];
static int count;           /* AH=0Eh result */
static int current;         /* AH=19h result */
static int queried[26];     /* AH=36h calls per drive */
static int nqueries;
static int order[64];       /* drives in query order */
static int ncurrent, ncount;

static int mock_current(void)
{
    ncurrent++;
    return current;
}

static int mock_count(void)
{
    ncount++;
    return count;
}

static int mock_query(int drive, struct diskfree *df)
{
    if (drive < 0 || drive >= 26)
        return -1;
    queried[drive]++;
    if (nqueries < 64)
        order[nqueries] = drive;
    nqueries++;
    if (!present[drive])
        return -1;
    /* 1 MiB, a quarter used */
    df->spc = 1;
    df->bps = 1024;
    df->total = 1024;
    df->avail = 768;
    return 0;
}

static const struct df_ops ops = { mock_current, mock_count, mock_query };

static struct options o;
static char *args[16];
static char out[2048];

static void reset(void)
{
    memset(present, 0, sizeof present);
    memset(queried, 0, sizeof queried);
    memset(order, 0, sizeof order);
    nqueries = ncurrent = ncount = 0;
    count = 5;
    current = 2;
    present[0] = present[1] = present[2] = 1;
}

/* Run df with a NULL-terminated argument list; output lands in out. */
static int run(const char *first, ...)
{
    va_list ap;
    const char *a;
    int argc = 0, rc;
    FILE *fo = tmpfile(), *fe = tmpfile();
    size_t n;

    args[argc++] = "df";
    va_start(ap, first);
    for (a = first; a != NULL; a = va_arg(ap, const char *))
        args[argc++] = (char *)a;
    va_end(ap);
    args[argc] = NULL;

    if (parse_options(argc, args, &o, fe) != OPT_RUN)
        rc = -1;
    else
        rc = df_run(&o, argc, args, &ops, fo, fe);
    rewind(fo);
    n = fread(out, 1, sizeof out - 1, fo);
    out[n] = '\0';
    fclose(fo);
    fclose(fe);
    return rc;
}

#define HDR  "Drive      1K-blocks       Used      Avail  Use%\n"
#define ROW(d)  d ":              1024        256        768   25%\n"

int main(void)
{
    int i;

    /* default scan: C: up to the drive count, never A: or B: */
    reset();
    present[3] = present[4] = 1;
    CHECK(run(NULL) == RC_OK);
    CHECK(queried[0] == 0 && queried[1] == 0);
    CHECK(queried[2] == 1 && queried[3] == 1 && queried[4] == 1);
    CHECK(nqueries == 3);
    CHECK(ncurrent == 0);
    CHECK(strcmp(out, HDR ROW("C") ROW("D") ROW("E")) == 0);

    /* gaps are skipped quietly */
    reset();
    count = 26;
    present[25] = 1;
    CHECK(run(NULL) == RC_OK);
    CHECK(queried[0] == 0 && queried[1] == 0 && nqueries == 24);
    CHECK(strcmp(out, HDR ROW("C") ROW("Z")) == 0);

    /* other options do not change the floppy policy */
    reset();
    CHECK(run("-h", NULL) == RC_OK);
    CHECK(queried[0] == 0 && queried[1] == 0);
    CHECK(strcmp(out, "Drive           Size       Used      Avail  Use%\n"
                      "C:            1.0MiB   256.0KiB   768.0KiB   25%\n")
          == 0);
    reset();
    CHECK(run("-k", NULL) == RC_OK);
    CHECK(queried[0] == 0 && queried[1] == 0);

    /* no usable drives */
    reset();
    present[2] = 0;
    CHECK(run(NULL) == RC_FAIL);
    CHECK(queried[0] == 0 && queried[1] == 0);
    CHECK(strcmp(out, "") == 0);
    reset();
    count = 2;                  /* only A: and B: */
    CHECK(run(NULL) == RC_FAIL);
    CHECK(nqueries == 0);

    /* the count is clamped to 26 letters */
    reset();
    count = 255;
    CHECK(run(NULL) == RC_OK);
    CHECK(nqueries == 24);
    for (i = 2; i < 26; i++)
        CHECK(queried[i] == 1);

    /* -a starts at A: */
    reset();
    CHECK(run("-a", NULL) == RC_OK);
    CHECK(queried[0] == 1 && queried[1] == 1 && queried[2] == 1);
    CHECK(order[0] == 0 && order[1] == 1 && order[2] == 2);
    CHECK(strcmp(out, HDR ROW("A") ROW("B") ROW("C")) == 0);
    reset();
    present[0] = 0;
    CHECK(run("-AH", NULL) == RC_OK);
    CHECK(queried[0] == 1);

    /* explicit operands: exactly those drives, in order, A: included */
    reset();
    CHECK(run("A:", NULL) == RC_OK);
    CHECK(nqueries == 1 && queried[0] == 1 && ncount == 0);
    CHECK(strcmp(out, HDR ROW("A")) == 0);
    reset();
    CHECK(run("b:", NULL) == RC_OK);
    CHECK(nqueries == 1 && queried[1] == 1);
    reset();
    CHECK(run("C:", "A:", "C:\\FOO\\BAR", NULL) == RC_OK);
    CHECK(nqueries == 3);
    CHECK(order[0] == 2 && order[1] == 0 && order[2] == 2);
    CHECK(strcmp(out, HDR ROW("C") ROW("A") ROW("C")) == 0);

    /* paths without a drive use the current drive, asked once */
    reset();
    current = 1;
    CHECK(run(".", "FOO", "\\BAR", NULL) == RC_OK);
    CHECK(ncurrent == 1 && nqueries == 3 && queried[1] == 3);
    reset();
    present[3] = 1;
    CHECK(run("D:FOO", NULL) == RC_OK);
    CHECK(ncurrent == 0 && queried[3] == 1 && queried[2] == 0);

    /* after --, a dash operand is a path on the current drive */
    reset();
    CHECK(run("--", "-h", NULL) == RC_OK);
    CHECK(nqueries == 1 && queried[2] == 1);
    CHECK(strcmp(out, HDR ROW("C")) == 0);

    /* a missing explicit drive fails but the rest are still reported */
    reset();
    CHECK(run("Q:", NULL) == RC_FAIL);
    CHECK(strcmp(out, "") == 0);
    reset();
    CHECK(run("C:", "Q:", "A:", NULL) == RC_FAIL);
    CHECK(nqueries == 3);
    CHECK(strcmp(out, HDR ROW("C") ROW("A")) == 0);
    /* explicit operands never fall back to a scan */
    reset();
    CHECK(run("Q:", NULL) == RC_FAIL && ncount == 0 && nqueries == 1);

    /* a malformed operand is a usage error before any query */
    reset();
    CHECK(run("C:", "1:", NULL) == -1);
    CHECK(nqueries == 0 && ncurrent == 0 && ncount == 0);

    return UNIT_DONE();
}
