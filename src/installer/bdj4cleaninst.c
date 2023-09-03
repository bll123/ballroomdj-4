/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "dirop.h"
#include "fileop.h"
#include "osprocess.h"
#include "osutils.h"
#include "tmutil.h"
#include "sysvars.h"

int
main (int argc, const char *argv [])
{
  const char  *fn;
  size_t      len;
  int         count;

  if (argc < 3) {
    return 1;
  }

  sysvarsInit (argv [0]);

  /* argument 1: the plocal/bin directory */
  /* argument 2: the directory to delete */
  /* argument 3: flag */

  /* when started, the PATH variable on windows is pointing to the */
  /* installation dir's plocal.  Need to use the new installation's plocal */
  /* so that the installation dir can be removed. */
  if (isWindows () && argc == 3) {
    char    *path = NULL;
    size_t  sz = 16384;

    path = malloc (sz);
    snprintf (path, sz, "%s;", argv [1]);
    strlcat (path, getenv ("PATH"), sz);
    osSetEnv ("PATH", path);
    free (path);
  }

  if (argc == 3) {
    const char  *targv [10];

    targv [0] = argv [0];
    targv [1] = argv [1];
    targv [2] = argv [2];
    targv [3] = "1";
    targv [4] = NULL;
    osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
    exit (0);
  }

  fn = argv [2];
  len = strlen (fn);
  if (strcmp (fn + len - strlen (BDJ4_INST_DIR), BDJ4_INST_DIR) != 0) {
    return 1;
  }

  count = 0;
  while (fileopIsDirectory (fn)) {
    diropDeleteDir (fn);
    /* there can be a race condition with the insttest.sh script */
    /* if another sleep is done */
    if (isWindows () && fileopIsDirectory (fn)) {
      mssleep (500);
    }
    ++count;
    if (count > 20) {
      return 1;
    }
  }
  return 0;
}
