/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdj4ui.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "callback.h"
#include "genre.h"
#include "ilist.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "musicq.h"
#include "nlist.h"
#include "playlist.h"
#include "rating.h"
#include "songfav.h"
#include "songfilter.h"
#include "songlist.h"
#include "sortopt.h"
#include "status.h"
#include "tagdef.h"
#include "ui.h"
#include "uidance.h"
#include "uifavorite.h"
#include "uigenre.h"
#include "uilevel.h"
#include "uiplaylist.h"
#include "uirating.h"
#include "uisongfilter.h"
#include "uistatus.h"
#include "uiutils.h"

enum {
  UISF_LABEL_SORTBY,
  UISF_LABEL_SEARCH,
  UISF_LABEL_GENRE,
  UISF_LABEL_DANCE,
  UISF_LABEL_DANCE_RATING,
  UISF_LABEL_DANCE_LEVEL,
  UISF_LABEL_STATUS,
  UISF_LABEL_FAVORITE,
  UISF_LABEL_PLAY_STATUS,
  UISF_LABEL_MAX,
};

enum {
  UISF_CB_FILTER,
  UISF_CB_PLAYLIST_SEL,
  UISF_CB_SORT_BY_SEL,
  UISF_CB_GENRE_SEL,
  UISF_CB_DANCE_SEL,
  UISF_CB_MAX,
};


typedef struct uisongfilter {
  char              *playlistname;
  rating_t          *ratings;
  level_t           *levels;
  status_t          *status;
  sortopt_t         *sortopt;
  uiwcont_t         *parentwin;
  nlist_t           *options;
  callback_t        *callbacks [UISF_CB_MAX];
  callback_t        *applycb;
  callback_t        *danceselcb;
  songfilter_t      *songfilter;
  playlist_t        *playlist;
  uiwcont_t         *filterDialog;
  uiplaylist_t      *uiplaylist;
  uiwcont_t         *playlistdisp;
  uidropdown_t      *sortbyfilter;
  uidance_t         *uidance;
  uigenre_t         *uigenre;
  uientry_t         *searchentry;
  uirating_t        *uirating;
  uilevel_t         *uilevel;
  uistatus_t        *uistatus;
  uifavorite_t      *uifavorite;
  uiwcont_t         *playstatusswitch;
  uiwcont_t         *labels [UISF_LABEL_MAX];
  songfilterpb_t    dfltpbflag;
  int               danceIdx;
  bool              showplaylist : 1;
  bool              playlistsel : 1;
} uisongfilter_t;

/* song filter handling */
static void uisfDisableWidgets (uisongfilter_t *uisf);
static void uisfEnableWidgets (uisongfilter_t *uisf);
static void uisfCreateDialog (uisongfilter_t *uisf);
static bool uisfResponseHandler (void *udata, long responseid);
static void uisfUpdate (uisongfilter_t *uisf);
static bool uisfPlaylistSelectHandler (void *udata, long idx);
static bool uisfSortBySelectHandler (void *udata, long idx);
static bool uisfGenreSelectHandler (void *udata, long idx);
static bool uisfDanceSelectHandler (void *udata, long idx, int count);
static void uisfInitDisplay (uisongfilter_t *uisf);
static void uisfPlaylistSelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfSortBySelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfCreateSortByList (uisongfilter_t *uisf);
static void uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfUpdateFilterDialogDisplay (uisongfilter_t *uisf);

uisongfilter_t *
uisfInit (uiwcont_t *windowp, nlist_t *options, songfilterpb_t pbflag)
{
  uisongfilter_t *uisf;

  uisf = mdmalloc (sizeof (uisongfilter_t));

  uisf->playlistname = NULL;
  uisf->playlist = NULL;
  uisf->ratings = bdjvarsdfGet (BDJVDF_RATINGS);
  uisf->levels = bdjvarsdfGet (BDJVDF_LEVELS);
  uisf->status = bdjvarsdfGet (BDJVDF_STATUS);
  uisf->sortopt = sortoptAlloc ();
  uisf->applycb = NULL;
  uisf->danceselcb = NULL;
  uisf->parentwin = windowp;
  uisf->options = options;
  uisf->songfilter = songfilterAlloc ();
  songfilterSetSort (uisf->songfilter, nlistGetStr (options, SONGSEL_SORT_BY));
  songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE, pbflag);
  uisf->filterDialog = NULL;
  uisf->dfltpbflag = pbflag;
  uisf->uiplaylist = NULL;
  uisf->sortbyfilter = uiDropDownInit ();
  uisf->uidance = NULL;
  uisf->playlistdisp = NULL;
  uisf->searchentry = uiEntryInit (20, 100);
  uisf->uigenre = NULL;
  uisf->uirating = NULL;
  uisf->uilevel = NULL;
  uisf->uistatus = NULL;
  uisf->uifavorite = NULL;
  uisf->playstatusswitch = NULL;
  uisf->danceIdx = -1;
  uisf->showplaylist = false;
  uisf->playlistsel = false;
  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uisf->labels [i] = NULL;
  }
  for (int i = 0; i < UISF_CB_MAX; ++i) {
    uisf->callbacks [i] = NULL;
  }

  return uisf;
}

void
uisfFree (uisongfilter_t *uisf)
{
  if (uisf != NULL) {
    dataFree (uisf->playlistname);
    uiwcontFree (uisf->playlistdisp);
    uiwcontFree (uisf->filterDialog);
    uiplaylistFree (uisf->uiplaylist);
    uiDropDownFree (uisf->sortbyfilter);
    uiEntryFree (uisf->searchentry);
    uidanceFree (uisf->uidance);
    uigenreFree (uisf->uigenre);
    uiratingFree (uisf->uirating);
    uilevelFree (uisf->uilevel);
    uistatusFree (uisf->uistatus);
    uifavoriteFree (uisf->uifavorite);
    uiwcontFree (uisf->playstatusswitch);
    sortoptFree (uisf->sortopt);
    songfilterFree (uisf->songfilter);
    playlistFree (uisf->playlist);
    for (int i = 0; i < UISF_LABEL_MAX; ++i) {
      uiwcontFree (uisf->labels [i]);
    }
    for (int i = 0; i < UISF_CB_MAX; ++i) {
      callbackFree (uisf->callbacks [i]);
    }
    mdfree (uisf);
  }
}

void
uisfSetApplyCallback (uisongfilter_t *uisf, callback_t *applycb)
{
  if (uisf == NULL) {
    return;
  }
  uisf->applycb = applycb;
}

void
uisfSetDanceSelectCallback (uisongfilter_t *uisf, callback_t *danceselcb)
{
  if (uisf == NULL) {
    return;
  }
  uisf->danceselcb = danceselcb;
}

void
uisfShowPlaylistDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  uisf->showplaylist = true;
  uisfUpdateFilterDialogDisplay (uisf);
}

void
uisfHidePlaylistDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  uisf->showplaylist = false;
  uisfUpdateFilterDialogDisplay (uisf);
}

bool
uisfPlaylistInUse (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return false;
  }

  return songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST);
}

bool
uisfDialog (void *udata)
{
  uisongfilter_t *uisf = udata;
  int             x, y;

  logProcBegin (LOG_PROC, "uisfDialog");

  uisfCreateDialog (uisf);
  uisfInitDisplay (uisf);
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
    uiSwitchSetValue (uisf->playstatusswitch, uisf->dfltpbflag);
  }
  uiDialogShow (uisf->filterDialog);

  uisfUpdateFilterDialogDisplay (uisf);

  x = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_Y);
  uiWindowMove (uisf->filterDialog, x, y, -1);
  logProcEnd (LOG_PROC, "uisfDialog", "");
  return UICB_CONT;
}

void
uisfSetPlaylist (uisongfilter_t *uisf, char *plname)
{
  if (uisf == NULL) {
    return;
  }

  uiplaylistSetValue (uisf->uiplaylist, plname);
  dataFree (uisf->playlistname);
  uisf->playlistname = mdstrdup (plname);
  songfilterSetData (uisf->songfilter, SONG_FILTER_PLAYLIST, plname);
  songfilterSetNum (uisf->songfilter, SONG_FILTER_PL_TYPE,
      playlistGetType (plname));

  uisfUpdateFilterDialogDisplay (uisf);
}

void
uisfClearPlaylist (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  uiplaylistSetValue (uisf->uiplaylist, "");
  dataFree (uisf->playlistname);
  uisf->playlistname = NULL;
  songfilterClear (uisf->songfilter, SONG_FILTER_PLAYLIST);
  uisfUpdateFilterDialogDisplay (uisf);
}

void
uisfSetDanceIdx (uisongfilter_t *uisf, int danceIdx)
{
  if (uisf == NULL) {
    return;
  }

  uisf->danceIdx = danceIdx;
  uidanceSetValue (uisf->uidance, danceIdx);

  if (danceIdx >= 0) {
    songfilterSetNum (uisf->songfilter, SONG_FILTER_DANCE_IDX, danceIdx);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_DANCE_IDX);
  }
}

songfilter_t *
uisfGetSongFilter (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return NULL;
  }
  return uisf->songfilter;
}

/* internal routines */

static void
uisfInitDisplay (uisongfilter_t *uisf)
{
  char        *sortby;

  logProcBegin (LOG_PROC, "uisfInitFilterDisplay");

  /* this is run when the filter dialog is first started, */
  /* and after a reset */
  /* all items need to be set, as after a reset, they need to be updated */
  /* sort-by and dance are important, the others can be reset */

  uiplaylistSetValue (uisf->uiplaylist, "");
  if (uisf->playlistname != NULL) {
    uiplaylistSetValue (uisf->uiplaylist, uisf->playlistname);
  }

  sortby = songfilterGetSort (uisf->songfilter);
  uiDropDownSelectionSetStr (uisf->sortbyfilter, sortby);
  uidanceSetValue (uisf->uidance, uisf->danceIdx);
  uigenreSetValue (uisf->uigenre, -1);
  uiEntrySetValue (uisf->searchentry, "");
  uiratingSetValue (uisf->uirating, -1);
  uilevelSetValue (uisf->uilevel, -1);
  uistatusSetValue (uisf->uistatus, -1);
  uifavoriteSetValue (uisf->uifavorite, 0);
  logProcEnd (LOG_PROC, "uisfInitFilterDisplay", "");
}

static void
uisfPlaylistSelect (uisongfilter_t *uisf, ssize_t idx)
{
  const char  *str;
  pltype_t    pltype;

  logProcBegin (LOG_PROC, "uisfPlaylistSelect");
  if (idx >= 0) {
    str = uiplaylistGetValue (uisf->uiplaylist);
    songfilterSetData (uisf->songfilter, SONG_FILTER_PLAYLIST, (void *) str);

    pltype = playlistGetType (str);
    songfilterSetNum (uisf->songfilter, SONG_FILTER_PL_TYPE, pltype);
    if (pltype == PLTYPE_AUTO || pltype == PLTYPE_SEQUENCE) {
      playlistFree (uisf->playlist);
      uisf->playlist = playlistLoad (str, NULL);
      playlistSetSongFilter (uisf->playlist, uisf->songfilter);
    }
    uisfDisableWidgets (uisf);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_PLAYLIST);
    uisfEnableWidgets (uisf);
  }
  uisf->playlistsel = true;
  logProcEnd (LOG_PROC, "uisfPlaylistSelect", "");
}

static void
uisfSortBySelect (uisongfilter_t *uisf, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisfSortBySelect");
  if (idx >= 0) {
    songfilterSetSort (uisf->songfilter,
        uiDropDownGetString (uisf->sortbyfilter));
    nlistSetStr (uisf->options, SONGSEL_SORT_BY,
        uiDropDownGetString (uisf->sortbyfilter));
  }
  logProcEnd (LOG_PROC, "uisfSortBySelect", "");
}

static void
uisfCreateSortByList (uisongfilter_t *uisf)
{
  slist_t           *sortoptlist;

  logProcBegin (LOG_PROC, "uisfCreateSortByList");

  sortoptlist = sortoptGetList (uisf->sortopt);
  uiDropDownSetList (uisf->sortbyfilter, sortoptlist, NULL);
  logProcEnd (LOG_PROC, "uisfCreateSortByList", "");
}

static void
uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx)
{
  logProcBegin (LOG_PROC, "uisfGenreSelect");
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_GENRE, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_GENRE);
    }
  }
  logProcEnd (LOG_PROC, "uisfGenreSelect", "");
}

static void
uisfCreateDialog (uisongfilter_t *uisf)
{
  uiwcont_t    *vbox;
  uiwcont_t    *hbox;
  uiwcont_t    *uiwidgetp;
  uiwcont_t    *szgrp;          // labels
  uiwcont_t    *szgrpEntry;     // playlist, title, search
  uiwcont_t    *szgrpDD;        // drop-downs, not including title
  uiwcont_t    *szgrpSpinText;  // spinboxes, not including favorite
  uiutilsaccent_t accent;

  logProcBegin (LOG_PROC, "uisfCreateDialog");

  if (uisf->filterDialog != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpEntry = uiCreateSizeGroupHoriz ();
  szgrpDD = uiCreateSizeGroupHoriz ();
  szgrpSpinText = uiCreateSizeGroupHoriz ();

  uisf->callbacks [UISF_CB_FILTER] = callbackInitLong (
      uisfResponseHandler, uisf);
  uisf->filterDialog = uiCreateDialog (uisf->parentwin,
      uisf->callbacks [UISF_CB_FILTER],
      /* CONTEXT: song selection filter: title for the filter dialog */
      _("Filter Songs"),
      /* CONTEXT: song selection filter: filter dialog: closes the dialog */
      _("Close"),
      RESPONSE_CLOSE,
      /* CONTEXT: song selection filter: filter dialog: resets the selections */
      _("Reset"),
      RESPONSE_RESET,
      /* CONTEXT: song selection filter: filter dialog: applies the selections */
      _("Apply"),
      RESPONSE_APPLY,
      NULL
      );

  vbox = uiCreateVertBox ();
  uiDialogPackInDialog (uisf->filterDialog, vbox);

  /* accent color */
  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.label);

  uiwcontFree (hbox);
  /* begin line */

  /* playlist : only available for the music manager */
  /* in this case, the entire hbox will be shown/hidden */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uisf->playlistdisp = hbox;

  /* CONTEXT: song selection filter: a filter: select a playlist to work with (music manager) */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uisf->callbacks [UISF_CB_PLAYLIST_SEL] = callbackInitLong (
      uisfPlaylistSelectHandler, uisf);
  uisf->uiplaylist = uiplaylistCreate (uisf->filterDialog, hbox, PL_LIST_ALL);
  uiplaylistSetSelectCallback (uisf->uiplaylist, uisf->callbacks [UISF_CB_PLAYLIST_SEL]);
  /* looks bad if added to the size group */

  /* do not free hbox (playlistdisp) */
  /* begin line */

  /* sort-by : always available */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: song selection filter: a filter: select the method to sort the song selection display */
  uiwidgetp = uiCreateColonLabel (_("Sort by"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uisf->labels [UISF_LABEL_SORTBY] = uiwidgetp;

  uisf->callbacks [UISF_CB_SORT_BY_SEL] = callbackInitLong (
      uisfSortBySelectHandler, uisf);
  uiwidgetp = uiComboboxCreate (uisf->sortbyfilter,
      uisf->filterDialog, "", uisf->callbacks [UISF_CB_SORT_BY_SEL], uisf);
  uisfCreateSortByList (uisf);
  uiBoxPackStart (hbox, uiwidgetp);
  /* looks bad if added to the size group */

  uiwcontFree (hbox);
  /* begin line */

  /* search : always available */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  /* CONTEXT: song selection filter: a filter: filter the song selection with a search for text */
  uiwidgetp = uiCreateColonLabel (_("Search"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uisf->labels [UISF_LABEL_SEARCH] = uiwidgetp;

  uiEntryCreate (uisf->searchentry);
  uiwidgetp = uiEntryGetWidgetContainer (uisf->searchentry);
  uiWidgetAlignHorizStart (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpEntry, uiwidgetp);

  /* genre */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_GENRE].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_GENRE] = uiwidgetp;

    uisf->callbacks [UISF_CB_GENRE_SEL] = callbackInitLong (
        uisfGenreSelectHandler, uisf);
    uisf->uigenre = uigenreDropDownCreate (hbox, uisf->filterDialog, true);
    uigenreSetCallback (uisf->uigenre, uisf->callbacks [UISF_CB_GENRE_SEL]);
    /* looks bad if added to the size group */
  }

  /* dance : always available */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCE].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uisf->labels [UISF_LABEL_DANCE] = uiwidgetp;

  uisf->callbacks [UISF_CB_DANCE_SEL] = callbackInitLongInt (
      uisfDanceSelectHandler, uisf);
  uisf->uidance = uidanceDropDownCreate (hbox, uisf->filterDialog,
      /* CONTEXT: song selection filter: a filter: all dances are selected */
      UIDANCE_ALL_DANCES,  _("All Dances"), UIDANCE_PACK_START, 1);
  uidanceSetCallback (uisf->uidance, uisf->callbacks [UISF_CB_DANCE_SEL]);
  /* adding to the size group makes it look weird */

  /* rating : always available */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uisf->labels [UISF_LABEL_DANCE_RATING] = uiwidgetp;

  uisf->uirating = uiratingSpinboxCreate (hbox, UIRATING_ALL);
  uiratingSizeGroupAdd (uisf->uirating, szgrpSpinText);

  /* level */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCELEVEL)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCELEVEL].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_DANCE_LEVEL] = uiwidgetp;

    uisf->uilevel = uilevelSpinboxCreate (hbox, true);
    uilevelSizeGroupAdd (uisf->uilevel, szgrpSpinText);
  }

  /* status */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_STATUS].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_STATUS] = uiwidgetp;

    uisf->uistatus = uistatusSpinboxCreate (hbox, true);
    uistatusSizeGroupAdd (uisf->uistatus, szgrpSpinText);
  }

  /* favorite */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_FAVORITE)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_FAVORITE].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_FAVORITE] = uiwidgetp;

    uisf->uifavorite = uifavoriteSpinboxCreate (hbox);
  }

  /* status playable */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    /* CONTEXT: song selection filter: a filter: the song status is marked as playable */
    uiwidgetp = uiCreateColonLabel (_("Playable Status"));
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_PLAY_STATUS] = uiwidgetp;

    uisf->playstatusswitch = uiCreateSwitch (uisf->dfltpbflag);
    uiBoxPackStart (hbox, uisf->playstatusswitch);
  }

  /* the dialog doesn't have any space above the buttons */
  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);

  uiwidgetp = uiCreateLabel (" ");
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (vbox);
  uiwcontFree (hbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpEntry);
  uiwcontFree (szgrpDD);
  uiwcontFree (szgrpSpinText);

  logProcEnd (LOG_PROC, "uisfCreateDialog", "");
}

static bool
uisfResponseHandler (void *udata, long responseid)
{
  uisongfilter_t  *uisf = udata;
  int             x, y, ws;

  logProcBegin (LOG_PROC, "uisfResponseHandler");

  uiWindowGetPosition (uisf->filterDialog, &x, &y, &ws);
  nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_X, x);
  nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: del window");
      uiwcontFree (uisf->filterDialog);
      uisf->filterDialog = NULL;
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: close window");
      uiWidgetHide (uisf->filterDialog);
      break;
    }
    case RESPONSE_APPLY: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: apply");
      break;
    }
    case RESPONSE_RESET: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: reset");
      songfilterReset (uisf->songfilter);
      dataFree (uisf->playlistname);
      uisf->playlistname = NULL;
      playlistFree (uisf->playlist);
      uisf->playlist = NULL;
      uisf->danceIdx = -1;
      uidanceSetValue (uisf->uidance, uisf->danceIdx);
      if (uisf->danceselcb != NULL) {
        callbackHandlerLong (uisf->danceselcb, uisf->danceIdx);
      }
      uisfInitDisplay (uisf);
      if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
        uiSwitchSetValue (uisf->playstatusswitch, uisf->dfltpbflag);
      }
      break;
    }
  }

  if (responseid != RESPONSE_DELETE_WIN && responseid != RESPONSE_CLOSE) {
    uisfUpdate (uisf);
    uisf->playlistsel = false;
  }
  if (responseid == RESPONSE_DELETE_WIN || responseid == RESPONSE_CLOSE) {
    if (uisf->playlistsel) {
      dataFree (uisf->playlistname);
      uisf->playlistname = NULL;
    }
  }

  return UICB_CONT;
}

void
uisfUpdate (uisongfilter_t *uisf)
{
  const char    *searchstr;
  int           idx;
  int           nval;


  /* playlist : if active, no other filters are used */
  if (uisf->showplaylist) {
    /* turn playlist back on */
    songfilterOn (uisf->songfilter, SONG_FILTER_PLAYLIST);

    if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
      uisfDisableWidgets (uisf);

      /* no other filters are applicable when a playlist filter is active */
      if (uisf->applycb != NULL) {
        callbackHandler (uisf->applycb);
      }
      return;
    }
  }

  if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
    uisfDisableWidgets (uisf);
  } else {
    uisfEnableWidgets (uisf);
  }

  /* search : always active */
  searchstr = uiEntryGetValue (uisf->searchentry);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisf->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating : always active */
  idx = uiratingGetValue (uisf->uirating);
  if (idx >= 0) {
    songfilterSetNum (uisf->songfilter, SONG_FILTER_RATING, idx);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_RATING);
  }

  /* dance level */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCELEVEL)) {
    idx = uilevelGetValue (uisf->uilevel);
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_LEVEL_LOW, idx);
      songfilterSetNum (uisf->songfilter, SONG_FILTER_LEVEL_HIGH, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_LEVEL_LOW);
      songfilterClear (uisf->songfilter, SONG_FILTER_LEVEL_HIGH);
    }
  }

  /* status */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS)) {
    idx = uistatusGetValue (uisf->uistatus);
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_STATUS);
    }
  }

  /* favorite */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_FAVORITE)) {
    idx = uifavoriteGetValue (uisf->uifavorite);
    if (idx != SONG_FAVORITE_NONE) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_FAVORITE, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_FAVORITE);
    }
  }

  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
    nval = uiSwitchGetValue (uisf->playstatusswitch);
  } else {
    nval = uisf->dfltpbflag;
  }
  if (nval) {
    songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE,
        SONG_FILTER_FOR_PLAYBACK);
  } else {
    /* can just clear the filter, or set it to for-selection */
    songfilterClear (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE);
  }

  if (uisf->applycb != NULL) {
    callbackHandler (uisf->applycb);
  }
  logProcEnd (LOG_PROC, "uisfResponseHandler", "");
}

/* internal routines */

static void
uisfDisableWidgets (uisongfilter_t *uisf)
{
  if (uisf->filterDialog == NULL) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetSetState (uisf->labels [i], UIWIDGET_DISABLE);
  }
  uiDropDownSetState (uisf->sortbyfilter, UIWIDGET_DISABLE);
  uiEntrySetState (uisf->searchentry, UIWIDGET_DISABLE);
  uidanceSetState (uisf->uidance, UIWIDGET_DISABLE);
  uigenreSetState (uisf->uigenre, UIWIDGET_DISABLE);
  uiratingSetState (uisf->uirating, UIWIDGET_DISABLE);
  uilevelSetState (uisf->uilevel, UIWIDGET_DISABLE);
  uistatusSetState (uisf->uistatus, UIWIDGET_DISABLE);
  uifavoriteSetState (uisf->uifavorite, UIWIDGET_DISABLE);
  uiWidgetSetState (uisf->playstatusswitch, UIWIDGET_DISABLE);
}

static void
uisfEnableWidgets (uisongfilter_t *uisf)
{
  if (uisf->filterDialog == NULL) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetSetState (uisf->labels [i], UIWIDGET_ENABLE);
  }
  uiDropDownSetState (uisf->sortbyfilter, UIWIDGET_ENABLE);
  uiEntrySetState (uisf->searchentry, UIWIDGET_ENABLE);
  uidanceSetState (uisf->uidance, UIWIDGET_ENABLE);
  uigenreSetState (uisf->uigenre, UIWIDGET_ENABLE);
  uiratingSetState (uisf->uirating, UIWIDGET_ENABLE);
  uilevelSetState (uisf->uilevel, UIWIDGET_ENABLE);
  uistatusSetState (uisf->uistatus, UIWIDGET_ENABLE);
  uifavoriteSetState (uisf->uifavorite, UIWIDGET_ENABLE);
  uiWidgetSetState (uisf->playstatusswitch, UIWIDGET_ENABLE);
}

static bool
uisfPlaylistSelectHandler (void *udata, long idx)
{
  uisongfilter_t *uisf = udata;

  uisfPlaylistSelect (uisf, idx);
  return UICB_CONT;
}

static bool
uisfSortBySelectHandler (void *udata, long idx)
{
  uisongfilter_t *uisf = udata;

  uisfSortBySelect (uisf, idx);
  return UICB_CONT;
}

static bool
uisfGenreSelectHandler (void *udata, long idx)
{
  uisongfilter_t *uisf = udata;

  uisfGenreSelect (uisf, idx);
  return UICB_CONT;
}

/* count is not used */
static bool
uisfDanceSelectHandler (void *udata, long idx, int count)
{
  uisongfilter_t  *uisf = udata;

  uisf->danceIdx = idx;
  uisfSetDanceIdx (uisf, idx);
  if (uisf->danceselcb != NULL) {
    callbackHandlerLong (uisf->danceselcb, idx);
  }
  return UICB_CONT;
}

static void
uisfUpdateFilterDialogDisplay (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  if (uisf->showplaylist) {
    songfilterOn (uisf->songfilter, SONG_FILTER_PLAYLIST);

    if (uisf->filterDialog == NULL) {
      return;
    }

    uiWidgetShowAll (uisf->playlistdisp);
  }

  if (! uisf->showplaylist) {
    songfilterOff (uisf->songfilter, SONG_FILTER_PLAYLIST);

    if (uisf->filterDialog == NULL) {
      return;
    }

    uiWidgetHide (uisf->playlistdisp);
  }

  if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
    uisfDisableWidgets (uisf);
  } else {
    uisfEnableWidgets (uisf);
  }
}

