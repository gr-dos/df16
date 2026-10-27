#include <string.h>
#include "options.h"
#include "out.h"

static int is_letter(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int lower(int c)
{
    return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

/* ASCII case-insensitive equality */
static int same(const char *a, const char *b)
{
    for (; *a && lower(*a) == lower(*b); a++, b++)
        ;
    return lower(*a) == lower(*b);
}

int operand_drive(const char *arg)
{
    const char *colon = strchr(arg, ':');

    if (arg[0] == '\0')
        return DRIVE_BAD;
    if (colon == NULL)
        return DRIVE_CURRENT;
    if (colon != arg + 1 || !is_letter(arg[0]) || strchr(colon + 1, ':'))
        return DRIVE_BAD;
    return lower(arg[0]) - 'a';
}

int parse_options(int argc, char **argv, struct options *o, FILE *err)
{
    int i, endopts = 0;
    const char *a;
    char opt[2];

    o->all = 0;
    o->human = 0;

    for (i = 1; i < argc; i++) {
        a = argv[i];

        if (endopts || a[0] != '-') {
            if (operand_drive(a) == DRIVE_BAD) {
                put(err, "df: invalid drive or path '", a, "'\n", NULL);
                return OPT_USAGE;
            }
            continue;
        }
        if (a[1] == '\0') {
            fputs("df: invalid option '-'\n", err);
            return OPT_USAGE;
        }
        if (a[1] == '-') {
            if (a[2] == '\0')
                endopts = 1;
            else if (same(a, "--help"))
                return OPT_HELP;
            else if (same(a, "--version"))
                return OPT_VERSION;
            else {
                put(err, "df: unrecognized option '", a, "'\n", NULL);
                return OPT_USAGE;
            }
            argv[i] = NULL;
            continue;
        }
        for (a++; *a; a++) {
            switch (lower(*a)) {
            case 'a':
                o->all = 1;
                break;
            case 'h':
                o->human = 1;
                break;
            case 'k':
                o->human = 0;
                break;
            case '?':
                return OPT_HELP;
            default:
                opt[0] = *a;
                opt[1] = '\0';
                put(err, "df: invalid option -- '", opt, "'\n", NULL);
                return OPT_USAGE;
            }
        }
        argv[i] = NULL;
    }
    return OPT_RUN;
}
