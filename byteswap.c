
#include <stdlib.h>
#include <stdio.h>

int isBigEndian()
{
  unsigned char EndianTest[2] = {1,0};
  short x = *(short *)EndianTest;

  if( x == 1 )
    return 0;
  else
    return 1;
}

