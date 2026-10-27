#ifndef OUT_H
#define OUT_H

#include <stdio.h>

/* Output without printf.  Portable: no DOS calls. */

/* Writes the strings, up to a null pointer, to fp. */
void put(FILE *fp, const char *s, ...);

#endif
