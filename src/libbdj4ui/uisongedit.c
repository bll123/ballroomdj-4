#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "log.h"
#include "uisongedit.h"
#include "ui.h"

uisongedit_t *
uisongeditInit (conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, nlist_t *options)
{
  uisongedit_t  *uisongedit;

  logProcBegin (LOG_PROC, "uisongeditInit");

  uisongedit = malloc (sizeof (uisongedit_t));
  assert (uisongedit != NULL);

  uisongedit->conn = conn;
  uisongedit->dispsel = dispsel;
  uisongedit->musicdb = musicdb;
  uisongedit->options = options;

  uisongeditUIInit (uisongedit);

  logProcEnd (LOG_PROC, "uisongeditInit", "");
  return uisongedit;
}

void
uisongeditFree (uisongedit_t *uisongedit)
{
  logProcBegin (LOG_PROC, "uisongeditFree");

  if (uisongedit != NULL) {
    uisongeditUIFree (uisongedit);
    free (uisongedit);
  }

  logProcEnd (LOG_PROC, "uisongeditFree", "");
}

void
uisongeditMainLoop (uisongedit_t *uisongedit)
{
  return;
}

void
uisongeditNewSelection (uisongedit_t *uisongedit, dbidx_t dbidx)
{
  uisongeditLoadData (uisongedit, dbidx);
}

