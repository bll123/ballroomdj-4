/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "log.h"
#include "ui.h"

void
confuiGenreSet (uiwcont_t *uitree,
    int editable, const char *genredisp, int clflag)
{
  logProcBegin ();
  uiTreeViewSetValues (uitree,
      CONFUI_GENRE_COL_EDITABLE, (treeint_t) editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, (treeint_t) clflag,
      TREE_VALUE_END);
  logProcEnd ("");
}

