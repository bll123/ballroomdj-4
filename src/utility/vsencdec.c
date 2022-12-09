#include <stdio.h>
#include <stdlib.h>

#include "vsencdec.h"

int
main (int argc, char *argv [])
{
  char    buff [100];

  if (argc != 2) {
    fprintf (stderr, "usage %s <str>\n", argv [0]);
    exit (1);
  }

  vsencdec (argv [1], buff, sizeof (buff));
  fprintf (stderr, "%s\n", buff);
  return 0;
}
