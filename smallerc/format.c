/*
 * format.c for Smaller C, whose 16-bit target has no long or long
 * long.  dfsize_t is four 16-bit words (see format.h), and the few
 * operations src/format.c needs are done a word or a bit at a time.
 * Each function follows its counterpart in src/format.c expression by
 * expression, wrapping modulo 2^64 the same way, so the two give the
 * same results; tests/test_format.c checks both.
 *
 * Portable C, so it also builds on the host (with DFSIZE_WORDS).
 */

#include <stdio.h>
#include <string.h>
#include "format.h"

/* Columns: drive, size, used, avail, use%.  Each number keeps at least
 * one space before it even if it outgrows its column. */
#define ROWFMT  "%-5s %14s %10s %10s %5s"

static const char *const units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
#define NUNITS  (sizeof units / sizeof units[0])

/* ------------------------------------------------------------------
 * 64-bit arithmetic.  Results go to the first argument, which may
 * also be an operand. */

static void set(dfsize_t *r, unsigned v)
{
    r->w[0] = (unsigned short)v;
    r->w[1] = r->w[2] = r->w[3] = 0;
}

static int is_zero(const dfsize_t *a)
{
    return (a->w[0] | a->w[1] | a->w[2] | a->w[3]) == 0;
}

static int less(const dfsize_t *a, const dfsize_t *b)
{
    int i;

    for (i = 3; i > 0 && a->w[i] == b->w[i]; i--)
        ;
    return a->w[i] < b->w[i];
}

/* r += a; the carry out */
static unsigned add(dfsize_t *r, const dfsize_t *a)
{
    unsigned i, c = 0;
    unsigned short w;

    for (i = 0; i < 4; i++) {
        w = (unsigned short)(r->w[i] + a->w[i] + c);
        c = c ? w <= r->w[i] : w < r->w[i];
        r->w[i] = w;
    }
    return c;
}

static void sub(dfsize_t *r, const dfsize_t *a)
{
    unsigned i, b = 0;
    unsigned short w;

    for (i = 0; i < 4; i++) {
        w = (unsigned short)(r->w[i] - a->w[i] - b);
        b = b ? w >= r->w[i] : w > r->w[i];
        r->w[i] = w;
    }
}

static void addu(dfsize_t *r, unsigned v)
{
    dfsize_t t;

    set(&t, v);
    add(r, &t);
}

static void subu(dfsize_t *r, unsigned v)
{
    dfsize_t t;

    set(&t, v);
    sub(r, &t);
}

/* r *= m, by shift and add */
static void mulu(dfsize_t *r, unsigned m)
{
    dfsize_t x = *r;

    set(r, 0);
    for (; m != 0; m >>= 1) {
        if (m & 1)
            add(r, &x);
        add(&x, &x);
    }
}

/* q = n / d and r = n % d, by shift and subtract.  d is not zero, and
 * q and r are different. */
static void divmod(dfsize_t *q, dfsize_t *r, const dfsize_t *n,
                   const dfsize_t *d)
{
    unsigned i, hi, out;

    *q = *n;
    set(r, 0);
    for (i = 0; i < 64; i++) {
        /* shift r:q left one bit */
        hi = add(q, q);
        out = add(r, r);
        r->w[0] |= hi;
        if (out || !less(r, d)) {
            sub(r, d);
            q->w[0] |= 1;
        }
    }
}

/* r = n / d; n % d */
static unsigned divu(dfsize_t *r, const dfsize_t *n, unsigned d)
{
    dfsize_t t, m;

    set(&t, d);
    divmod(r, &m, n, &t);
    return m.w[0];
}

static int lessu(const dfsize_t *a, unsigned v)
{
    dfsize_t t;

    set(&t, v);
    return less(a, &t);
}

/* ------------------------------------------------------------------ */

void df_sizes(const struct diskfree *df, struct dfsizes *s)
{
    dfsize_t cluster, t;
    unsigned avail = df->avail > df->total ? df->total : df->avail;

    set(&cluster, df->spc);
    mulu(&cluster, df->bps);

    s->total = cluster;
    mulu(&s->total, df->total);
    s->avail = cluster;
    mulu(&s->avail, avail);
    s->used = s->total;
    sub(&s->used, &s->avail);
    if (is_zero(&s->total)) {
        s->pct = -1;
    } else {
        /* (used * 100 + total - 1) / total */
        t = s->used;
        mulu(&t, 100);
        add(&t, &s->total);
        subu(&t, 1);
        divmod(&t, &cluster, &t, &s->total);
        s->pct = (int)t.w[0];
    }
}

void fmt_u64(char *buf, dfsize_t v)
{
    char tmp[21];
    int n = 0;

    do {
        tmp[n++] = (char)('0' + (int)divu(&v, &v, 10));
    } while (!is_zero(&v));
    while (n > 0)
        *buf++ = tmp[--n];
    *buf = '\0';
}

void fmt_kib(char *buf, dfsize_t bytes)
{
    addu(&bytes, 1023);
    divu(&bytes, &bytes, 1024);
    fmt_u64(buf, bytes);
}

void fmt_human(char *buf, dfsize_t bytes)
{
    dfsize_t div, tenths, rem, t;
    unsigned u, d;

    if (lessu(&bytes, 1024)) {
        fmt_u64(buf, bytes);
        strcat(buf, units[0]);
        return;
    }
    /* tenths of the unit, rounded up; move up a unit once it would
     * show as 1024.0:
     * tenths = bytes / div * 10 + (bytes % div * 10 + div - 1) / div */
    set(&div, 1024);
    for (u = 1; ; u++, mulu(&div, 1024)) {
        divmod(&tenths, &rem, &bytes, &div);
        mulu(&tenths, 10);
        mulu(&rem, 10);
        add(&rem, &div);
        subu(&rem, 1);
        divmod(&t, &rem, &rem, &div);
        add(&tenths, &t);
        if (lessu(&tenths, 10240) || u == NUNITS - 1)
            break;
    }
    d = divu(&tenths, &tenths, 10);
    fmt_u64(buf, tenths);
    buf += strlen(buf);
    *buf++ = '.';
    *buf++ = (char)('0' + (int)d);
    strcpy(buf, units[u]);
}

void fmt_header(char *buf, int human)
{
    sprintf(buf, ROWFMT, "Drive", human ? "Size" : "1K-blocks",
            "Used", "Avail", "Use%");
}

void fmt_row(char *buf, int drive, const struct dfsizes *s, int human)
{
    char name[3], size[24], used[24], avail[24], pct[12];
    void (*fmt)(char *, dfsize_t) = human ? fmt_human : fmt_kib;

    name[0] = (char)('A' + drive);
    name[1] = ':';
    name[2] = '\0';
    fmt(size, s->total);
    fmt(used, s->used);
    fmt(avail, s->avail);
    if (s->pct < 0)
        strcpy(pct, "-");
    else
        sprintf(pct, "%d%%", s->pct);
    sprintf(buf, ROWFMT, name, size, used, avail, pct);
}
