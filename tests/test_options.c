/* Host-side tests for options.c. */

#include <stdarg.h>
#include <stdio.h>
#include "options.h"
#include "unit.h"

static struct options o;
static char *args[16];
static FILE *err;

/* Parse a NULL-terminated argument list (without argv[0]). */
static int parse(const char *first, ...)
{
    va_list ap;
    const char *a;
    int argc = 0;

    args[argc++] = "df";
    va_start(ap, first);
    for (a = first; a != NULL; a = va_arg(ap, const char *))
        args[argc++] = (char *)a;
    va_end(ap);
    args[argc] = NULL;
    return parse_options(argc, args, &o, err);
}

int main(void)
{
    err = tmpfile();
    if (err == NULL) {
        perror("tmpfile");
        return 1;
    }

    /* defaults */
    CHECK(parse(NULL) == OPT_RUN);
    CHECK(o.all == 0 && o.human == 0);

    /* -h is human-readable, never help */
    CHECK(parse("-h", NULL) == OPT_RUN && o.human == 1);
    CHECK(parse("-H", NULL) == OPT_RUN && o.human == 1);
    CHECK(parse("-k", NULL) == OPT_RUN && o.human == 0);
    CHECK(parse("-K", NULL) == OPT_RUN && o.human == 0);
    CHECK(parse("-a", NULL) == OPT_RUN && o.all == 1 && o.human == 0);
    CHECK(parse("-A", NULL) == OPT_RUN && o.all == 1);

    /* help and version, any case */
    CHECK(parse("-?", NULL) == OPT_HELP);
    CHECK(parse("--help", NULL) == OPT_HELP);
    CHECK(parse("--HELP", NULL) == OPT_HELP);
    CHECK(parse("--Help", NULL) == OPT_HELP);
    CHECK(parse("-a?", NULL) == OPT_HELP);
    CHECK(parse("C:", "--help", NULL) == OPT_HELP);
    CHECK(parse("--version", NULL) == OPT_VERSION);
    CHECK(parse("--VERSION", NULL) == OPT_VERSION);
    CHECK(parse("-h", "--version", NULL) == OPT_VERSION);

    /* grouping, and the last of -h/-k wins */
    CHECK(parse("-ah", NULL) == OPT_RUN && o.all && o.human);
    CHECK(parse("-AH", NULL) == OPT_RUN && o.all && o.human);
    CHECK(parse("-hk", NULL) == OPT_RUN && o.human == 0);
    CHECK(parse("-kh", NULL) == OPT_RUN && o.human == 1);
    CHECK(parse("-h", "-k", NULL) == OPT_RUN && o.human == 0);
    CHECK(parse("-k", "-h", NULL) == OPT_RUN && o.human == 1);
    CHECK(parse("-h", "C:", "-k", NULL) == OPT_RUN && o.human == 0);

    /* options are removed, operands stay in order */
    CHECK(parse("-h", "C:", "-a", ".", NULL) == OPT_RUN);
    CHECK(args[1] == NULL && args[3] == NULL);
    CHECK(args[2] != NULL && args[4] != NULL);

    /* -- ends options */
    CHECK(parse("--", "-h", NULL) == OPT_RUN && o.human == 0);
    CHECK(args[1] == NULL && args[2] != NULL);
    CHECK(parse("--", "--help", NULL) == OPT_RUN);
    CHECK(parse("--", "--", NULL) == OPT_RUN && args[2] != NULL);

    /* usage errors */
    CHECK(parse("-x", NULL) == OPT_USAGE);
    CHECK(parse("-ax", NULL) == OPT_USAGE);
    CHECK(parse("--bogus", NULL) == OPT_USAGE);
    CHECK(parse("--h", NULL) == OPT_USAGE);
    CHECK(parse("--helpx", NULL) == OPT_USAGE);
    CHECK(parse("-", NULL) == OPT_USAGE);
    CHECK(parse("-help", NULL) == OPT_USAGE);   /* -h -e ... */
    CHECK(parse("1:", NULL) == OPT_USAGE);
    CHECK(parse("C:", "1:", NULL) == OPT_USAGE);
    CHECK(parse("", NULL) == OPT_USAGE);
    CHECK(parse("--", "1:", NULL) == OPT_USAGE);

    /* drive selection */
    CHECK(operand_drive("C:") == 2);
    CHECK(operand_drive("c:") == 2);
    CHECK(operand_drive("A:") == 0);
    CHECK(operand_drive("b:") == 1);
    CHECK(operand_drive("Z:") == 25);
    CHECK(operand_drive("z:") == 25);
    CHECK(operand_drive("C:\\") == 2);
    CHECK(operand_drive("C:\\FOO\\BAR") == 2);
    CHECK(operand_drive("c:/foo/bar") == 2);
    CHECK(operand_drive("D:FOO") == 3);
    CHECK(operand_drive("d:foo\\bar") == 3);
    CHECK(operand_drive(".") == DRIVE_CURRENT);
    CHECK(operand_drive("..") == DRIVE_CURRENT);
    CHECK(operand_drive("FOO") == DRIVE_CURRENT);
    CHECK(operand_drive("\\FOO\\BAR") == DRIVE_CURRENT);
    CHECK(operand_drive("C") == DRIVE_CURRENT);
    CHECK(operand_drive("-x") == DRIVE_CURRENT);
    CHECK(operand_drive("") == DRIVE_BAD);
    CHECK(operand_drive("1:") == DRIVE_BAD);
    CHECK(operand_drive(":") == DRIVE_BAD);
    CHECK(operand_drive(":FOO") == DRIVE_BAD);
    CHECK(operand_drive("@:") == DRIVE_BAD);
    CHECK(operand_drive("[:") == DRIVE_BAD);
    CHECK(operand_drive("`:") == DRIVE_BAD);
    CHECK(operand_drive("{:") == DRIVE_BAD);
    CHECK(operand_drive("\x81:") == DRIVE_BAD);
    CHECK(operand_drive("CD:") == DRIVE_BAD);
    CHECK(operand_drive("FOO:") == DRIVE_BAD);
    CHECK(operand_drive("C::") == DRIVE_BAD);
    CHECK(operand_drive("C:FOO:") == DRIVE_BAD);
    CHECK(operand_drive("\\FOO:") == DRIVE_BAD);

    /* the default scan skips A: and B: unless -a */
    CHECK(parse(NULL) == OPT_RUN && SCAN_FIRST(&o) == 2);
    CHECK(parse("-a", NULL) == OPT_RUN && SCAN_FIRST(&o) == 0);

    fclose(err);
    return UNIT_DONE();
}
