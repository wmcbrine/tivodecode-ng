/* useful hex manipulation routines */
/* Copyright C Qualcomm Inc 1997 */

#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

#include "hexlib.hxx"

#define COLS 16

int hexbulk(uint8_t *buf, int n)
{
    int i = 0;
    int j = 0;
    char ch;
    char hexdigit[5];
    std::string hexstr = "";
    std::string strstr = "";
    
    while (i < n)
    {
        for (j = 0; (j < COLS) && (i < n); j++, i++)
        {
            ch = buf[i];
            if (std::isspace(ch))
                ch = ' ';
            else if (!std::isprint(ch))
                ch = '.';

            std::sprintf(hexdigit, "%02x ", buf[i]);
            hexstr += hexdigit;
            strstr += ch;
        }

        while (j < COLS)
        {
            hexstr += "   ";
            j++;
        }

        std::cerr << hexstr << " " << strstr << "\n";
    }

    return 0;
}
