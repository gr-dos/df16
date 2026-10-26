#include <stdarg.h>
#include "out.h"

void put(FILE *fp, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    for (; s != NULL; s = va_arg(ap, const char *))
        fputs(s, fp);
    va_end(ap);
}
