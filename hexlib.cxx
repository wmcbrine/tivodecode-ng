/* useful hex manipulation routines */
/* Copyright C Qualcomm Inc 1997 */
/* @(#)hexlib.c	1.1 (AHAG) 02/08/23 */

#ifdef HAVE_CONFIG_H
# include "tdconfig.h"
#endif

#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "hexlib.hxx"

#define COLS 16

int
hexbulk(unsigned char *buf, int n)
{
    int i = 0;
    int j = 0;
    char ch;
    char hexdigit[5];
    char strdigit[5];
    char hexstr[100];
    char strstr[100];
    
    while (i < n)
    {
        std::memset(hexstr, 0, 100);
        std::memset(strstr, 0, 100);
	
        for (j = 0; (j < COLS) && (i < n); j++, i++)
        {
            if (std::isspace(buf[i]))
                ch = ' ';
            else if (std::isprint(buf[i]))
                ch = buf[i];
            else
                ch = '.';

            std::sprintf(hexdigit, "%02x ", buf[i]);
            std::sprintf(strdigit, "%c", ch);
            std::strcat(hexstr, hexdigit);
            std::strcat(strstr, strdigit);
        }

        while (j < COLS)
        {
            std::strcat(hexstr, "   ");	
            j++;
        }

        std::cerr << hexstr << " " << strstr << "\n";
    }

    return 0;
}
