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

char *
filedataReadAll (char *fname)
{
  FILE        *fh;
  struct stat statbuf;
  int         rc;
  size_t      len;
  char        *data;

  rc = stat (fname, &statbuf);
  if (rc != 0) {
    return NULL;
  }
  fh = fopen (fname, "r");
  data = malloc ((size_t) statbuf.st_size + 1);
  assert (data != NULL);
  len = fread (data, 1, (size_t) statbuf.st_size, fh);
  assert ((statbuf.st_size == 0 && len == 0) || len > 0);
  data [len] = '\0';
  fclose (fh);

  return data;
}

