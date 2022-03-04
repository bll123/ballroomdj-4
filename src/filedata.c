#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "filedata.h"
#include "fileop.h"

char *
filedataReadAll (char *fname)
{
  FILE        *fh;
  ssize_t     len;
  char        *data;

  len = fileopSize (fname);
  if (len < 0) {
    return NULL;
  }
  fh = fopen (fname, "r");
  data = malloc (len + 1);
  assert (data != NULL);
  len = fread (data, 1, len, fh);
  assert (len > 0);
  data [len] = '\0';
  fclose (fh);

  return data;
}

