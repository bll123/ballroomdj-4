#include <stdio.h>
#include <stdlib.h>

#include "bdj4main.h"

int
main (int argc, char *argv[])
{
  bdj4startup (argc, argv);
  /* do other stuff */
  bdj4finalizeStartup ();
  /* enter main loops */
  bdj4shutdown ();
}
