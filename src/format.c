#include <string.h>
#include "format.h"

/* Appends s to p, padded with spaces to width: on the right if left is
 * nonzero, else on the left.  Returns the new end. */
static char *field(char *p, const char *s, int width, int left)
{
    int n = (int)strlen(s);

    if (!left)
        for (; n < width; width--)
            *p++ = ' ';
    memcpy(p, s, (size_t)n);
    p += n;
    if (left)
        for (; n < width; width--)
            *p++ = ' ';
    *p = '\0';
    return p;
}

/* Columns: drive (5, left-aligned), size (14), used (10), avail (10),
 * use% (5), separated by one space.  Each number keeps at least one
 * space before it even if it outgrows its column. */
static void row(char *buf, const char *drive, const char *size,
                const char *used, const char *avail, const char *pct)
{
    char *p = field(buf, drive, 5, 1);

    *p++ = ' ';
    p = field(p, size, 14, 0);
    *p++ = ' ';
    p = field(p, used, 10, 0);
    *p++ = ' ';
    p = field(p, avail, 10, 0);
    *p++ = ' ';
    field(p, pct, 5, 0);
}

static const char *const units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
#define NUNITS  (sizeof units / sizeof units[0])

void df_sizes(const struct diskfree *df, struct dfsizes *s)
{
    dfsize_t cluster = (dfsize_t)df->spc * df->bps;
    unsigned avail = df->avail > df->total ? df->total : df->avail;

    s->total = cluster * df->total;
    s->avail = cluster * avail;
    s->used = s->total - s->avail;
    if (s->total == 0)
        s->pct = -1;
    else
        s->pct = (int)((s->used * 100 + s->total - 1) / s->total);
}

void fmt_u64(char *buf, dfsize_t v)
{
    char tmp[21];
    int n = 0;

    do {
        tmp[n++] = (char)('0' + (int)(v % 10));
        v /= 10;
    } while (v != 0);
    while (n > 0)
        *buf++ = tmp[--n];
    *buf = '\0';
}

void fmt_kib(char *buf, dfsize_t bytes)
{
    fmt_u64(buf, (bytes + 1023) / 1024);
}

void fmt_human(char *buf, dfsize_t bytes)
{
    dfsize_t div = 1024, tenths;
    unsigned u;

    if (bytes < 1024) {
        fmt_u64(buf, bytes);
        strcat(buf, units[0]);
        return;
    }
    /* tenths of the unit, rounded up; move up a unit once it would
     * show as 1024.0.  Splitting off the remainder keeps bytes * 10
     * from overflowing for any 64-bit value. */
    for (u = 1; ; u++, div *= 1024) {
        tenths = bytes / div * 10 + (bytes % div * 10 + div - 1) / div;
        if (tenths < 10240 || u == NUNITS - 1)
            break;
    }
    fmt_u64(buf, tenths / 10);
    buf += strlen(buf);
    *buf++ = '.';
    *buf++ = (char)('0' + (int)(tenths % 10));
    strcpy(buf, units[u]);
}

void fmt_header(char *buf, int human)
{
    row(buf, "Drive", human ? "Size" : "1K-blocks", "Used", "Avail", "Use%");
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
    else {
        fmt_u64(pct, (dfsize_t)s->pct);
        strcat(pct, "%");
    }
    row(buf, name, size, used, avail, pct);
}
