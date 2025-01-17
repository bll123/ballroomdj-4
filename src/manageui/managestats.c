/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
#include <time.h>

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "manageui.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "song.h"
#include "tagdef.h"
#include "tmutil.h"
#include "ui.h"

enum {
  STATS_COLS = 5,
  STATS_PER_COL = 9,
  STATS_MAX_DISP = (STATS_COLS * STATS_PER_COL) * 2,
};

typedef struct managestats {
  conn_t      *conn;
  musicdb_t   *musicdb;
  uiwcont_t   *vboxmain;
  uiwcont_t   *dancedisp [STATS_MAX_DISP];
  nlist_t     *dancecounts;
  uiwcont_t   *songcountdisp;
  uiwcont_t   *tottimedisp;
  int         songcount;
  long        tottime;
} managestats_t;

static void manageStatsDisplayStats (managestats_t *managestats);


managestats_t *
manageStatsInit (conn_t *conn, musicdb_t *musicdb)
{
  managestats_t *managestats;

  managestats = mdmalloc (sizeof (managestats_t));
  managestats->conn = conn;
  managestats->musicdb = musicdb;
  managestats->vboxmain = NULL;
  managestats->songcountdisp = NULL;
  managestats->tottimedisp = NULL;
  managestats->songcount = 0;
  managestats->tottime = 0;
  managestats->dancecounts = NULL;
  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    managestats->dancedisp [i] = NULL;
  }

  return managestats;
}

void
manageStatsFree (managestats_t *managestats)
{
  if (managestats != NULL) {
    uiwcontFree (managestats->vboxmain);
    nlistFree (managestats->dancecounts);
    for (int i = 0; i < STATS_MAX_DISP; ++i) {
      uiwcontFree (managestats->dancedisp [i]);
    }
    uiwcontFree (managestats->songcountdisp);
    uiwcontFree (managestats->tottimedisp);
    mdfree (managestats);
  }
}

void
manageStatsSetDatabase (managestats_t *managestats, musicdb_t *musicdb)
{
  managestats->musicdb = musicdb;
}

uiwcont_t *
manageBuildUIStats (managestats_t *managestats)
{
  uiwcont_t   *uiwidgetp;
  uiwcont_t   *hbox;
  uiwcont_t   *chbox;

  managestats->vboxmain = uiCreateVertBox ();

  /* Number of songs */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (managestats->vboxmain, hbox);
  uiWidgetSetMarginTop (hbox, 2);

  /* CONTEXT: statistics: Label for number of songs in song list */
  uiwidgetp = uiCreateColonLabel (_("Songs"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 2);
  uiwcontFree (uiwidgetp);

  managestats->songcountdisp = uiCreateLabel ("");
  uiLabelAlignEnd (managestats->songcountdisp);
  uiBoxPackStart (hbox, managestats->songcountdisp);

  /* total time (same horiz row) */
  /* CONTEXT: statistics: Label for total song list duration */
  uiwidgetp = uiCreateColonLabel (_("Total Time"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiWidgetSetMarginEnd (uiwidgetp, 2);
  uiWidgetSetMarginStart (uiwidgetp, 10);
  uiwcontFree (uiwidgetp);

  managestats->tottimedisp = uiCreateLabel ("");
  uiLabelAlignEnd (managestats->tottimedisp);
  uiBoxPackStart (hbox, managestats->tottimedisp);

  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    managestats->dancedisp [i] = uiCreateLabel ("");
    if (i % 2 == 1) {
      uiWidgetSetSizeRequest (managestats->dancedisp [i], 30, -1);
      uiLabelAlignEnd (managestats->dancedisp [i]);
    }
  }

  /* horizontal box to hold the columns */
  chbox = uiCreateHorizBox ();
  uiBoxPackStart (managestats->vboxmain, chbox);
  uiWidgetSetMarginTop (chbox, 2);

  for (int i = 0; i < STATS_COLS; ++i) {
    uiwcont_t  *vbox;

    /* vertical box for each column */
    vbox = uiCreateVertBox ();
    uiBoxPackStart (chbox, vbox);
    uiWidgetSetMarginTop (vbox, 2);
    uiWidgetSetMarginStart (vbox, 2);
    uiWidgetSetMarginEnd (vbox, 8);

    for (int j = 0; j < STATS_PER_COL; ++j) {
      int   idx;

      idx = i * STATS_PER_COL * 2 + j * 2;
      uiwcontFree (hbox);
      hbox = uiCreateHorizBox ();
      uiBoxPackStart (vbox, hbox);
      uiBoxPackStart (hbox, managestats->dancedisp [idx]);
      uiBoxPackEnd (hbox, managestats->dancedisp [idx + 1]);
    }

    uiwcontFree (vbox);
  }

  uiwcontFree (hbox);
  uiwcontFree (chbox);

  return managestats->vboxmain;
}

void
manageStatsProcessData (managestats_t *managestats, mp_musicqupdate_t *musicqupdate)
{
  int               ci;
  ilistidx_t        danceIdx;
  song_t            *song;
  nlist_t           *dcounts;
  nlistidx_t        iteridx;
  mp_musicqupditem_t   *musicqupditem;

  ci = musicqupdate->mqidx;
  managestats->songcount = 0;
  managestats->tottime = 0;

  if (ci != MUSICQ_SL) {
    return;
  }

  managestats->tottime = musicqupdate->tottime;

  dcounts = nlistAlloc ("stats", LIST_ORDERED, NULL);

  nlistStartIterator (musicqupdate->dispList, &iteridx);
  while ((musicqupditem = nlistIterateValueData (musicqupdate->dispList, &iteridx)) != NULL) {
    song = dbGetByIdx (managestats->musicdb, musicqupditem->dbidx);
    if (song == NULL) {
      continue;
    }
    danceIdx = songGetNum (song, TAG_DANCE);
    nlistIncrement (dcounts, danceIdx);
    ++managestats->songcount;
  }

  nlistFree (managestats->dancecounts);
  managestats->dancecounts = dcounts;

  /* this only needs to be called when the tab is displayed */
  manageStatsDisplayStats (managestats);
}

static void
manageStatsDisplayStats (managestats_t *managestats)
{
  char        tbuff [60];
  int         count;
  slist_t     *danceList;
  const char  *dancedisp;
  dance_t     *dances;
  slistidx_t  diteridx;
  ilistidx_t  didx;
  int         val;


  tmutilToMS (managestats->tottime, tbuff, sizeof (tbuff));
  uiLabelSetText (managestats->tottimedisp, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%d", managestats->songcount);
  uiLabelSetText (managestats->songcountdisp, tbuff);

  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    uiLabelSetText (managestats->dancedisp [i], "");
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);
  slistStartIterator (danceList, &diteridx);
  count = 0;
  while ((dancedisp = slistIterateKey (danceList, &diteridx)) != NULL) {
    didx = slistGetNum (danceList, dancedisp);
    val = nlistGetNum (managestats->dancecounts, didx);
    if (val > 0) {
      uiLabelSetText (managestats->dancedisp [count], dancedisp);
      ++count;
      snprintf (tbuff, sizeof (tbuff), "%d", val);
      uiLabelSetText (managestats->dancedisp [count], tbuff);
      ++count;
    }
  }
}

