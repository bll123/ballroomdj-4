#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdjvarsdf.h"
#include "dance.h"
#include "log.h"
#include "uiutils.h"

void
uiutilsCreateDanceList (uiutilsdropdown_t *dropdown, char *selectLabel)
{
  dance_t           *dances;
  slist_t           *danceList;

  logProcBegin (LOG_PROC, "uiutilsCreateDanceList");

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);

  uiutilsDropDownSetNumList (dropdown, danceList, selectLabel);
  logProcEnd (LOG_PROC, "uiutilsCreateDanceList", "");
}

