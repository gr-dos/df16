/* Host-side tests for format.c. */

#include <string.h>
#include "format.h"
#include "unit.h"

static char buf[FMT_LINESIZE];

static const char *kib(dfsize_t bytes)
{
    fmt_kib(buf, bytes);
    return buf;
}

static const char *human(dfsize_t bytes)
{
    fmt_human(buf, bytes);
    return buf;
}

static struct dfsizes sizes(unsigned spc, unsigned avail, unsigned bps,
                            unsigned total)
{
    struct diskfree df;
    struct dfsizes s;

    df.spc = spc;
    df.avail = avail;
    df.bps = bps;
    df.total = total;
    df_sizes(&df, &s);
    return s;
}

#define KIB(n)  ((dfsize_t)(n) * 1024)
#define MIB(n)  (KIB(n) * 1024)
#define GIB(n)  (MIB(n) * 1024)
#define TIB(n)  (GIB(n) * 1024)

int main(void)
{
    struct dfsizes s;

    /* decimal */
    fmt_u64(buf, 0);
    CHECK(strcmp(buf, "0") == 0);
    fmt_u64(buf, 65535);
    CHECK(strcmp(buf, "65535") == 0);
    fmt_u64(buf, 4294967296ULL);
    CHECK(strcmp(buf, "4294967296") == 0);
    fmt_u64(buf, 18446744073709551615ULL);
    CHECK(strcmp(buf, "18446744073709551615") == 0);

    /* arithmetic: a 16-bit product would wrap, and so would 32 bits */
    s = sizes(64, 0, 512, 65535);          /* 32 KiB clusters, 2 GiB */
    CHECK(s.total == 2147450880ULL);
    CHECK(s.used == s.total && s.avail == 0 && s.pct == 100);
    s = sizes(128, 65535, 512, 65535);     /* 64 KiB clusters: > 4 GiB */
    CHECK(s.total == 4294901760ULL);
    CHECK(s.avail == s.total && s.used == 0 && s.pct == 0);
    s = sizes(65535, 65535, 65535, 65535);
    CHECK(s.total == 281462092005375ULL);  /* 65535^3 */
    CHECK(s.pct == 0);

    /* typical FAT16 volume: 2 KiB clusters */
    s = sizes(4, 54415, 512, 62453);
    CHECK(s.total == 127903744ULL);
    CHECK(s.avail == 111441920ULL);
    CHECK(s.used == 16461824ULL);
    CHECK(s.pct == 13);                    /* 12.87% rounds up */

    /* percent rounds up: one used cluster of many is 1% */
    s = sizes(1, 9999, 512, 10000);
    CHECK(s.pct == 1);
    s = sizes(1, 0, 512, 10000);
    CHECK(s.pct == 100);
    s = sizes(1, 50, 512, 100);
    CHECK(s.pct == 50);
    s = sizes(1, 1, 512, 3);
    CHECK(s.pct == 67);

    /* zero capacity: no percentage */
    s = sizes(0, 0, 512, 100);
    CHECK(s.total == 0 && s.used == 0 && s.avail == 0 && s.pct == -1);
    s = sizes(4, 0, 512, 0);
    CHECK(s.total == 0 && s.pct == -1);
    s = sizes(4, 10, 0, 100);
    CHECK(s.total == 0 && s.pct == -1);

    /* more free than total clusters is clamped, never underflows */
    s = sizes(4, 200, 512, 100);
    CHECK(s.avail == s.total && s.used == 0 && s.pct == 0);
    s = sizes(4, 200, 512, 0);
    CHECK(s.total == 0 && s.avail == 0 && s.used == 0 && s.pct == -1);

    /* KiB blocks round up */
    CHECK(strcmp(kib(0), "0") == 0);
    CHECK(strcmp(kib(1), "1") == 0);
    CHECK(strcmp(kib(512), "1") == 0);
    CHECK(strcmp(kib(1024), "1") == 0);
    CHECK(strcmp(kib(1025), "2") == 0);
    CHECK(strcmp(kib(2147450880ULL), "2097120") == 0);
    CHECK(strcmp(kib(281462092005375ULL), "274865324224") == 0);

    /* human-readable: bytes below 1 KiB, then one decimal, rounded up */
    CHECK(strcmp(human(0), "0B") == 0);
    CHECK(strcmp(human(1), "1B") == 0);
    CHECK(strcmp(human(1023), "1023B") == 0);
    CHECK(strcmp(human(1024), "1.0KiB") == 0);
    CHECK(strcmp(human(1025), "1.1KiB") == 0);
    CHECK(strcmp(human(1536), "1.5KiB") == 0);
    CHECK(strcmp(human(KIB(1023)), "1023.0KiB") == 0);
    CHECK(strcmp(human(KIB(1023) + 1), "1023.1KiB") == 0);
    /* would round up to 1024.0KiB: shown as the next unit instead */
    CHECK(strcmp(human(MIB(1) - 1), "1.0MiB") == 0);
    CHECK(strcmp(human(MIB(1)), "1.0MiB") == 0);
    CHECK(strcmp(human(MIB(1) + 1), "1.1MiB") == 0);
    CHECK(strcmp(human(127903744ULL), "122.0MiB") == 0);
    CHECK(strcmp(human(16461824ULL), "15.7MiB") == 0);
    CHECK(strcmp(human(111441920ULL), "106.3MiB") == 0);
    CHECK(strcmp(human(MIB(512)), "512.0MiB") == 0);
    CHECK(strcmp(human(GIB(1) - 1), "1.0GiB") == 0);
    CHECK(strcmp(human(2147450880ULL), "2.0GiB") == 0);
    CHECK(strcmp(human(GIB(1023)), "1023.0GiB") == 0);
    CHECK(strcmp(human(TIB(1)), "1.0TiB") == 0);
    CHECK(strcmp(human(281462092005375ULL), "256.0TiB") == 0);
    /* TiB is the largest unit */
    CHECK(strcmp(human(TIB(2048)), "2048.0TiB") == 0);

    /* rows and headers */
    fmt_header(buf, 0);
    CHECK(strcmp(buf, "Drive      1K-blocks       Used      Avail  Use%") == 0);
    fmt_header(buf, 1);
    CHECK(strcmp(buf, "Drive           Size       Used      Avail  Use%") == 0);
    s = sizes(4, 54415, 512, 62453);
    fmt_row(buf, 2, &s, 0);
    CHECK(strcmp(buf, "C:            124906      16076     108830   13%") == 0);
    fmt_row(buf, 2, &s, 1);
    CHECK(strcmp(buf, "C:          122.0MiB    15.7MiB   106.3MiB   13%") == 0);
    s = sizes(0, 0, 0, 0);
    fmt_row(buf, 25, &s, 0);
    CHECK(strcmp(buf, "Z:                 0          0          0     -") == 0);
    /* the widest possible row still fits and keeps its columns apart */
    s = sizes(65535, 0, 65535, 65535);
    fmt_row(buf, 0, &s, 0);
    CHECK(strcmp(buf, "A:      274865324224 274865324224          0  100%") == 0);
    CHECK(strlen(buf) < 80);

    return UNIT_DONE();
}
