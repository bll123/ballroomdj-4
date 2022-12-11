/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "pathbld.h"
#include "ui.h"
#include "uiapp.h"

void
bdj4AppInitializeUI (void)
{
  const char  *uifont;
  char        fn [MAXPATHLEN];

  uiUIInitialize ();
  uifont = bdjoptGetStr (OPT_MP_UIFONT);
  uiSetUIFont (uifont);
  pathbldMakePath (fn, sizeof (fn),
      BDJ4_CSS_FN, BDJ4_CSS_EXT, PATHBLD_MP_DREL_DATA);
  uiLoadApplicationCSS (fn);
}
