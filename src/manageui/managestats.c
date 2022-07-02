#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "bdj4intl.h"
#include "manageui.h"
#include "musicdb.h"
#include "musicq.h"
#include "song.h"
#include "tagdef.h"
#include "ui.h"

typedef struct managestats {
  conn_t    *conn;
  musicdb_t *musicdb;
  UIWidget  vboxmain;
  UIWidget  songsdisp;
  UIWidget  tottimedisp;
  int       songcount;
  time_t    tottime;
} managestats_t;

static void manageStatsProcessData (managestats_t *managestats, char *data);


managestats_t *
manageStatsInit (conn_t *conn, musicdb_t *musicdb)
{
  managestats_t *managestats;

  managestats = malloc (sizeof (managestats_t));
  managestats->conn = conn;
  managestats->musicdb = musicdb;
  uiutilsUIWidgetInit (&managestats->vboxmain);
  uiutilsUIWidgetInit (&managestats->songsdisp);
  uiutilsUIWidgetInit (&managestats->tottimedisp);
  managestats->tottime = 0;

  return managestats;
}

void
manageStatsFree (managestats_t *managestats)
{
  if (managestats != NULL) {
    free (managestats);
  }
}

UIWidget *
manageBuildUIStats (managestats_t *managestats)
{
  UIWidget    uiwidget;
  UIWidget    hbox;

  uiCreateVertBox (&managestats->vboxmain);

  /* Number of songs */
  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&managestats->vboxmain, &hbox);

  uiCreateColonLabel (&uiwidget, _("Songs"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&managestats->songsdisp, "");
  uiLabelAlignEnd (&managestats->songsdisp);
  uiBoxPackStart (&hbox, &managestats->songsdisp);

  /* total time (same horiz row) */
  uiCreateColonLabel (&uiwidget, _("Total Time"));
  uiWidgetSetMarginStart (&uiwidget, uiBaseMarginSz * 10);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateLabel (&managestats->tottimedisp, "");
  uiLabelAlignEnd (&managestats->tottimedisp);
  uiBoxPackStart (&hbox, &managestats->tottimedisp);

  return &managestats->vboxmain;
}

void
manageStatsProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, managestats_t *managestats)
{
  char          *targs = NULL;

  if (args != NULL) {
    targs = strdup (args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MANAGEUI:
    case ROUTE_PLAYERUI: {
      switch (msg) {
        case MSG_MUSIC_QUEUE_DATA: {
          manageStatsProcessData (managestats, targs);
          // dbgdisp = true;
          break;
        }
        default: {
          break;
        }
      }
    }
    default: {
      break;
    }
  }

  if (targs != NULL) {
    free (targs);
  }
}

static void
manageStatsProcessData (managestats_t *managestats, char *data)
{
  int               ci;
  char              *p;
  char              *tokstr;
  dbidx_t           dbidx;
  ilistidx_t        danceIdx;
  song_t            *song;
  long              qDuration;

  managestats->songcount = 0;
  managestats->tottime = 0;

  p = strtok_r (data, MSG_ARGS_RS_STR, &tokstr);
  ci = atoi (p);
  if (ci != MUSICQ_A) {
    return;
  }
  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  qDuration = atol (p);

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  while (p != NULL) {
    /* dispidx */
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    /* uniqueidx */
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    /* database idx */
    dbidx = atol (p);
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    /* pause flag */

    song = dbGetByIdx (managestats->musicdb, dbidx);
    danceIdx = songGetNum (song, TAG_DANCE);
    ++managestats->songcount;

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  }
}

