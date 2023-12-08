/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "bdj4arg.h"
#include "check_bdj.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "osrandom.h"
#include "osdirutil.h"
#include "sysvars.h"

int
main (int argc, char *argv [])
{
  SRunner *sr = NULL;
  int     number_failed = 0;
  FILE    *fh;
  bdj4arg_t   *bdj4arg;
  const char  *targ;

  fh = fopen ("data/locale.txt", "w");
  fputs ("en_GB\n", fh);
  fclose (fh);

#if BDJ4_MEM_DEBUG
  mdebugInit ("chk");
  // mdebugSetVerbose ();
#endif

  bdj4arg = bdj4argInit (argc, argv);

  sRandom ();
  targ = bdj4argGet (bdj4arg, 0, argv [0]);
  sysvarsInit (targ);
  localeInit ();

  if (osChangeDir (sysvarsGetStr (SV_BDJ4_DIR_DATATOP)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4_DIR_DATATOP));
    bdj4argCleanup (bdj4arg);
    return 1;
  }

  /* macos's logging is really slow and affects the check suite */
  if (! isMacOS ()) {
    logStart ("check_all", "chk", LOG_ALL & ~LOG_PROC);
  }

  sr = srunner_create (NULL);
  check_libcommon (sr);
  check_libbasic (sr);
  check_libaudiosrc (sr);
  check_libbdj4 (sr);
  /* if the durations are needed */
  srunner_set_xml (sr, "tmp/check.xml");
  // srunner_set_log (sr, "tmp/check.log");
  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);

  localeCleanup ();
  logEnd ();
  bdj4argCleanup (bdj4arg);
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
