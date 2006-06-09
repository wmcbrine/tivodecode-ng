/* useful hex manipulation routines */
/* Copyright C Qualcomm Inc 1997 */
/* @(#)hexlib.c	1.1 (AHAG) 02/08/23 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hexlib.h"

int	nerrors;

static char	*hex = "0123456789abcdef";
#define HEX(c) ((int)(strchr(hex, (c)) - hex))

int
hexprint(const char *s, unsigned char *p, int n)
{
    printf("%14s:", s);
    while (--n >= 0) {
	if (n % 20 == 19)
	    printf("\n%14s ", "");
	printf(" %02x", *p++);
    }
    printf("\n");
    return 0;
}

int
hexread(unsigned char *buf, char *p, int n)
{
    int		i;

    while (--n >= 0) {
	while (*p == ' ') ++p;
	i = HEX(*p++) << 4;
	i += HEX(*p++);
	*buf++ = i;
    }
    return 0;
}

int
hexcheck(unsigned char *buf, char *p, int n)
{
    int		i;

    while (--n >= 0) {
	while (*p == ' ') ++p;
	i = HEX(*p++) << 4;
	i += HEX(*p++);
	if (*buf++ != i) {
	    printf("Expected %02x, got %02x.\n", i, buf[-1]);
	    ++nerrors;
	}
    }
    return nerrors;
}

#define COLS 20

int
hexbulk(unsigned char *buf, int n)
{
    int		i;

    for (i = 0; i < n; ++i)
	printf("%02x%c", buf[i], (i%COLS == (COLS-1) ? '\n' : ' '));
    if (i % COLS != 0)
	putchar('\n');
    return 0;
}
