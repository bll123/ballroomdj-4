#include "bdjconfig.h"

#include <stdio.h>

#include "musicdb.h"

int
main (int argc, char *argv[])
{
  dbOpen ("data/musicdb.dat");
  return 0;
}
