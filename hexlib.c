/* useful hex manipulation routines */
/* Copyright C Qualcomm Inc 1997 */
/* @(#)hexlib.c	1.1 (AHAG) 02/08/23 */

#ifdef HAVE_CONFIG_H
# include "tdconfig.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#include "hexlib.h"
#include <ctype.h>

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

#define COLS 16

int
hexbulk(unsigned char *buf, int n)
{
    int		i=0;
    int		j=0;
    char 	ch;
    char 	hexdigit[5];
    char 	strdigit[5];
    char 	hexstr[100];
    char 	strstr[100];

	while ( i<n )
	{
		memset(hexstr, 0, 100);
		memset(strstr, 0, 100);

		for(j=0; (j<COLS) && (i<n); j++, i++ )
		{
			if ( isspace(buf[i]) )
				ch = ' ';
			else if ( isprint( buf[i]))
				ch = buf[i];
			else
				ch = '.';

			sprintf( hexdigit, "%02x ", buf[i] );
			sprintf( strdigit, "%c",    ch);
			strcat( hexstr, hexdigit );
			strcat( strstr, strdigit );
		}

		while(j<COLS)
		{
			strcat( hexstr, "   " );
			j++;
		}

		printf("%s %s\n", hexstr, strstr);
	}

    return 0;
}
