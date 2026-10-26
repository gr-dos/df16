#ifndef FORMAT_H
#define FORMAT_H

/*
 * Capacity arithmetic and output formatting.  Portable: no DOS calls.
 *
 * A DOS free-space query gives four 16-bit values, so byte counts can
 * reach 65535^3 (about 2^48).  dfsize_t must be at least 64 bits;
 * Open Watcom provides unsigned long long for 16-bit targets with
 * integer-only helper routines.
 *
 * Smaller C's 16-bit target has no long or long long, so there it is
 * four 16-bit words, low first, with the arithmetic done by
 * smallerc/format.c instead of format.c.  DFSIZE_WORDS selects that
 * for the host tests.
 */

#if defined(__SMALLER_C__) || defined(DFSIZE_WORDS)
typedef struct { unsigned short w[4]; } dfsize_t;
#else
typedef unsigned long long dfsize_t;
#endif

/* The raw INT 21h AH=36h result. */
struct diskfree {
    unsigned spc;           /* AX: sectors per cluster */
    unsigned avail;         /* BX: free clusters */
    unsigned bps;           /* CX: bytes per sector */
    unsigned total;         /* DX: total clusters */
};

struct dfsizes {
    dfsize_t total;         /* bytes */
    dfsize_t used;
    dfsize_t avail;
    int pct;                /* percent used, rounded up; -1 if no capacity */
};

/* Big enough for any header or row. */
#define FMT_LINESIZE  80

/* Byte counts from a query.  Free clusters are clamped to total. */
void df_sizes(const struct diskfree *df, struct dfsizes *s);

/* v in decimal.  buf needs 21 bytes. */
void fmt_u64(char *buf, dfsize_t v);

/* bytes as 1 KiB blocks, rounded up. */
void fmt_kib(char *buf, dfsize_t bytes);

/* bytes human-readable: "512B", "1.5KiB", "122.0MiB", ... rounded up. */
void fmt_human(char *buf, dfsize_t bytes);

/* The column header line and one drive's line (0 = A:), without the
 * line ending. */
void fmt_header(char *buf, int human);
void fmt_row(char *buf, int drive, const struct dfsizes *s, int human);

#endif
