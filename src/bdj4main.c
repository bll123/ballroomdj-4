#include <stdio.h>
#include <stdlib.h>

#include "bdj4init.h"

int
main (int argc, char *argv[])
{
  bdj4startup (argc, argv);
  /* enter main loops */
  bdj4shutdown ();
  return 0;
}
