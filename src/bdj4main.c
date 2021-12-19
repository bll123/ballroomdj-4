#include "bdjconfig.h"

#include <stdio.h>

#include "musicdb.h"
#include "tagdef.h"

int
main (int argc, char *argv[])
{
  tagdefInit ();
  dbOpen ("data/musicdb.dat");
  return 0;
}
