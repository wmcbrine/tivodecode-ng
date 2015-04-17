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

	   		sprintf(hexdigit, "%02x ", buf[i] );	   			
	   		sprintf(strdigit, "%c",    ch);
			strcat(hexstr, hexdigit ); 
			strcat(strstr, strdigit );
		}
		
		while(j<COLS)
		{
			strcat( hexstr, "   " );	
			j++;
		}
		
		fprintf(stderr,"%s %s\n", hexstr, strstr);
	}		
				
    return 0;
}
