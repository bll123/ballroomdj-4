#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "locatebdj3.h"

int
main (int argc, char *argv [])
{
  char      *dir = NULL;

  dir = locatebdj3 ();
  if (dir != NULL && *dir) {
    printf ("%s\n", dir);
    free (dir);
  } else {
    printf ("none\n");
  }
}
