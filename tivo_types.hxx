
#ifndef __TIVO_TYPES_HXX__
#define __TIVO_TYPES_HXX__

#ifdef HAVE_CONFIG_H
#include "tdconfig.h"
#endif

#include <iostream>

typedef unsigned int   UINT32;
typedef int            INT32;
typedef unsigned short UINT16;
typedef short          INT16;
typedef unsigned char  UINT8;
typedef char           INT8;
typedef char           CHAR;
typedef char           BOOL;

#define TRUE  1
#define FALSE 0

#define PRINT_QUALCOMM_MSG() std::cerr << "Encryption by QUALCOMM ;)\n\n"

#endif /* TIVO_TYPES_HXX__ */

/* vi:set ai ts=4 sw=4 expandtab: */
