/*
 * Print df's rows (1 KiB and -h) for many free-space query results:
 * every combination of some edge values, then pseudo-random ones.
 * tests/rows.sh builds this with the host compiler, Open Watcom and
 * Smaller C and checks that all three print the same, which tests each
 * compiler's 64-bit arithmetic (or smallerc/format.c's) on DOS.
 */

#include <stdio.h>
#include "format.h"

static const unsigned edges[] = { 0, 1, 2, 512, 32768u, 65535u };
#define NEDGES  (sizeof edges / sizeof edges[0])
#define NRANDOM 400

static unsigned seed = 1;

/* 16 bits from a 16-bit LCG, the same on any host */
static unsigned next(void)
{
    seed = (seed * 25173u + 13849u) & 0xffffu;
    return seed;
}

static void put(unsigned spc, unsigned avail, unsigned bps, unsigned total)
{
    struct diskfree df;
    struct dfsizes s;
    char line[FMT_LINESIZE];

    df.spc = spc;
    df.avail = avail;
    df.bps = bps;
    df.total = total;
    df_sizes(&df, &s);
    printf("%u %u %u %u\n", spc, avail, bps, total);
    fmt_row(line, 0, &s, 0);
    printf("%s\n", line);
    fmt_row(line, 0, &s, 1);
    printf("%s\n", line);
}

int main(void)
{
    unsigned a, b, c, d, i;

    for (a = 0; a < NEDGES; a++)
        for (b = 0; b < NEDGES; b++)
            for (c = 0; c < NEDGES; c++)
                for (d = 0; d < NEDGES; d++)
                    put(edges[a], edges[b], edges[c], edges[d]);
    for (i = 0; i < NRANDOM; i++) {
        a = next();
        b = next();
        c = next();
        d = next();
        /* mostly no more free than total clusters, and half with
         * realistic powers of two per cluster and sector */
        if (i % 4 != 0)
            b %= d + 1u;
        if (i % 2 == 0) {
            a = 1u << (a % 8);
            c = 128u << (c % 4);
        }
        put(a, b, c, d);
    }
    return 0;
}
