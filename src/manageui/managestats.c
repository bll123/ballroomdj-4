/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
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
  uiwcont_t   dancedisp [STATS_MAX_DISP];
  nlist_t     *dancecounts;
  uiwcont_t   songcountdisp;
  uiwcont_t   tottimedisp;
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
  uiwcontInit (&managestats->songcountdisp);
  uiwcontInit (&managestats->tottimedisp);
  managestats->songcount = 0;
  managestats->tottime = 0;
  managestats->dancecounts = NULL;
  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    uiwcontInit (&managestats->dancedisp [i]);
  }

  return managestats;
}

void
manageStatsFree (managestats_t *managestats)
{
  if (managestats != NULL) {
    uiwcontFree (managestats->vboxmain);
    nlistFree (managestats->dancecounts);
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
  uiwcont_t   uiwidget;
  uiwcont_t   *hbox;
  uiwcont_t   *chbox;
  const char  *listingFont;

  managestats->vboxmain = uiCreateVertBox ();

  /* Number of songs */
  hbox = uiCreateHorizBox ();
  uiWidgetSetMarginTop (hbox, 2);
  uiBoxPackStart (managestats->vboxmain, hbox);

  /* CONTEXT: statistics: Label for number of songs in song list */
  uiCreateColonLabel (&uiwidget, _("Songs"));
  uiBoxPackStart (hbox, &uiwidget);

  uiCreateLabel (&managestats->songcountdisp, "");
  uiLabelAlignEnd (&managestats->songcountdisp);
  uiBoxPackStart (hbox, &managestats->songcountdisp);

  /* total time (same horiz row) */
  /* CONTEXT: statistics: Label for total song list duration */
  uiCreateColonLabel (&uiwidget, _("Total Time"));
  uiWidgetSetMarginStart (&uiwidget, 10);
  uiBoxPackStart (hbox, &uiwidget);

  uiCreateLabel (&managestats->tottimedisp, "");
  uiLabelAlignEnd (&managestats->tottimedisp);
  uiBoxPackStart (hbox, &managestats->tottimedisp);
  uiwcontFree (hbox);

  listingFont = bdjoptGetStr (OPT_MP_LISTING_FONT);

  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    uiCreateLabel (&managestats->dancedisp [i], "");
    if (i % 2 == 1) {
      uiWidgetSetSizeRequest (&managestats->dancedisp [i], 30, -1);
      uiLabelAlignEnd (&managestats->dancedisp [i]);
    }
    uiLabelSetFont (&managestats->dancedisp [i], listingFont);
  }

  /* horizontal box to hold the columns */
  chbox = uiCreateHorizBox ();
  uiWidgetSetMarginTop (chbox, 2);
  uiBoxPackStart (managestats->vboxmain, chbox);

  for (int i = 0; i < STATS_COLS; ++i) {
    uiwcont_t  *vbox;

    /* vertical box for each column */
    vbox = uiCreateVertBox ();
    uiWidgetSetMarginTop (vbox, 2);
    uiWidgetSetMarginStart (vbox, 2);
    uiWidgetSetMarginEnd (vbox, 8);
    uiBoxPackStart (chbox, vbox);

    for (int j = 0; j < STATS_PER_COL; ++j) {
      int   idx;

      idx = i * STATS_PER_COL * 2 + j * 2;
      hbox = uiCreateHorizBox ();
      uiBoxPackStart (vbox, hbox);
      uiBoxPackStart (hbox, &managestats->dancedisp [idx]);
      uiBoxPackEnd (hbox, &managestats->dancedisp [idx + 1]);
      uiwcontFree (hbox);
      hbox = NULL;
    }

    uiwcontFree (vbox);
  }

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
  char        *dancedisp;
  dance_t     *dances;
  slistidx_t  diteridx;
  ilistidx_t  didx;
  int         val;


  tmutilToMS (managestats->tottime, tbuff, sizeof (tbuff));
  uiLabelSetText (&managestats->tottimedisp, tbuff);

  snprintf (tbuff, sizeof (tbuff), "%d", managestats->songcount);
  uiLabelSetText (&managestats->songcountdisp, tbuff);

  for (int i = 0; i < STATS_MAX_DISP; ++i) {
    uiLabelSetText (&managestats->dancedisp [i], "");
  }

  dances = bdjvarsdfGet (BDJVDF_DANCES);
  danceList = danceGetDanceList (dances);
  slistStartIterator (danceList, &diteridx);
  count = 0;
  while ((dancedisp = slistIterateKey (danceList, &diteridx)) != NULL) {
    didx = slistGetNum (danceList, dancedisp);
    val = nlistGetNum (managestats->dancecounts, didx);
    if (val > 0) {
      uiLabelSetText (&managestats->dancedisp [count], dancedisp);
      ++count;
      snprintf (tbuff, sizeof (tbuff), "%d", val);
      uiLabelSetText (&managestats->dancedisp [count], tbuff);
      ++count;
    }
  }
}

