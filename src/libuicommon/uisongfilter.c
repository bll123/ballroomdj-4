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
#include "bdj4ui.h"
#include "bdjopt.h"
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
#include "uidd.h"
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
  UISF_CB_KEY,
  UISF_CB_MAX,
};

enum {
  UISF_W_DIALOG,
  UISF_W_PLAYLIST,
  UISF_W_SEARCH,
  UISF_W_PLAY_STATUS,
  UISF_W_KEY_HNDLR,
  UISF_W_MAX,
};

typedef struct uisongfilter {
  char              *playlistname;
  rating_t          *ratings;
  level_t           *levels;
  status_t          *status;
  sortopt_t         *sortopt;
  uiwcont_t         *wcont [UISF_W_MAX];
  uiwcont_t         *window;
  nlist_t           *options;
  callback_t        *callbacks [UISF_CB_MAX];
  callback_t        *applycb;
  callback_t        *danceselcb;
  songfilter_t      *songfilter;
  playlist_t        *playlist;
  uiplaylist_t      *uiplaylist;
  uidd_t            *ddsortby;
  ilist_t           *ddsortbylist;
  uidance_t         *uidance;
  uigenre_t         *uigenre;
  uirating_t        *uirating;
  uilevel_t         *uilevel;
  uistatus_t        *uistatus;
  uifavorite_t      *uifavorite;
  uiwcont_t         *labels [UISF_LABEL_MAX];
  songfilterpb_t    dfltpbflag;
  int               danceIdx;
  bool              showplaylist : 1;
} uisongfilter_t;

/* song filter handling */
static void uisfFreeDialog (uisongfilter_t *uisf);
static void uisfDisableWidgets (uisongfilter_t *uisf);
static void uisfEnableWidgets (uisongfilter_t *uisf);
static void uisfCreateDialog (uisongfilter_t *uisf);
static bool uisfResponseHandler (void *udata, int32_t responseid);
static void uisfUpdate (uisongfilter_t *uisf);
static int32_t uisfPlaylistSelectHandler (void *udata, const char *str);
static int32_t uisfSortBySelectHandler (void *udata, const char *sval);
static bool uisfGenreSelectHandler (void *udata, int32_t idx);
static bool uisfDanceSelectHandler (void *udata, int32_t idx, int32_t count);
static void uisfInitDisplay (uisongfilter_t *uisf);
static void uisfPlaylistSelect (uisongfilter_t *uisf, const char *fn);
static void uisfSortBySelect (uisongfilter_t *uisf, const char *sval);
static void uisfCreateSortByList (uisongfilter_t *uisf);
static void uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx);
static void uisfUpdateFilterDialogDisplay (uisongfilter_t *uisf);
static bool uisfKeyHandler (void *udata);

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
  for (int i = 0; i < UISF_W_MAX; ++i) {
    uisf->wcont [i] = NULL;
  }
  uisf->window = windowp;
  uisf->wcont [UISF_W_KEY_HNDLR] = uiEventAlloc ();
  uisf->options = options;
  uisf->ddsortby = NULL;
  uisf->ddsortbylist = NULL;
  uisf->songfilter = songfilterAlloc ();
  songfilterSetSort (uisf->songfilter, nlistGetStr (options, SONGSEL_SORT_BY));
  songfilterSetNum (uisf->songfilter, SONG_FILTER_STATUS_PLAYABLE, pbflag);
  uisf->dfltpbflag = pbflag;
  uisf->uiplaylist = NULL;
  uisf->uidance = NULL;
  uisf->uigenre = NULL;
  uisf->uirating = NULL;
  uisf->uilevel = NULL;
  uisf->uistatus = NULL;
  uisf->uifavorite = NULL;
  uisf->danceIdx = -1;
  uisf->showplaylist = false;
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
  if (uisf == NULL) {
    return;
  }

  dataFree (uisf->playlistname);
  uisfFreeDialog (uisf);
  sortoptFree (uisf->sortopt);
  songfilterFree (uisf->songfilter);
  playlistFree (uisf->playlist);
  ilistFree (uisf->ddsortbylist);
  for (int i = 0; i < UISF_CB_MAX; ++i) {
    callbackFree (uisf->callbacks [i]);
  }
  mdfree (uisf);
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

  logProcBegin ();

  uisfCreateDialog (uisf);
  uisfInitDisplay (uisf);
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
    uiSwitchSetValue (uisf->wcont [UISF_W_PLAY_STATUS], uisf->dfltpbflag);
  }
  uiDialogShow (uisf->wcont [UISF_W_DIALOG]);

  uisfUpdateFilterDialogDisplay (uisf);

  x = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_X);
  y = nlistGetNum (uisf->options, SONGSEL_FILTER_POSITION_Y);
  uiWindowMove (uisf->wcont [UISF_W_DIALOG], x, y, -1);
  logProcEnd ("");
  return UICB_CONT;
}

void
uisfSetPlaylist (uisongfilter_t *uisf, char *plname)
{
  if (uisf == NULL) {
    return;
  }
  dataFree (uisf->playlistname);
  uisf->playlistname = mdstrdup (plname);
  if (uisf->uiplaylist != NULL) {
    uiplaylistSetKey (uisf->uiplaylist, plname);
  }
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

  dataFree (uisf->playlistname);
  uisf->playlistname = NULL;
  if (uisf->uiplaylist != NULL) {
    uiplaylistSetKey (uisf->uiplaylist, "");
  }
  songfilterClear (uisf->songfilter, SONG_FILTER_PLAYLIST);
  uisfUpdateFilterDialogDisplay (uisf);
}

void
uisfSetDanceIdx (uisongfilter_t *uisf, int danceIdx)
{
  if (uisf == NULL) {
    return;
  }

  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCE)) {
    uisf->danceIdx = danceIdx;
    uidanceSetKey (uisf->uidance, danceIdx);

    if (danceIdx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_DANCE_IDX, danceIdx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_DANCE_IDX);
    }
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
uisfFreeDialog (uisongfilter_t *uisf)
{
  if (uisf == NULL) {
    return;
  }

  for (int i = 0; i < UISF_W_MAX; ++i) {
    uiwcontFree (uisf->wcont [i]);
    uisf->wcont [i] = NULL;
  }
  uiplaylistFree (uisf->uiplaylist);
  uisf->uiplaylist = NULL;
  uidanceFree (uisf->uidance);
  uisf->uidance = NULL;
  uigenreFree (uisf->uigenre);
  uisf->uigenre = NULL;
  uiratingFree (uisf->uirating);
  uisf->uirating = NULL;
  uilevelFree (uisf->uilevel);
  uisf->uilevel = NULL;
  uistatusFree (uisf->uistatus);
  uisf->uistatus = NULL;
  uifavoriteFree (uisf->uifavorite);
  uisf->uifavorite = NULL;
  uiddFree (uisf->ddsortby);
  uisf->ddsortby = NULL;
  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiwcontFree (uisf->labels [i]);
    uisf->labels [i] = NULL;
  }
}

static void
uisfInitDisplay (uisongfilter_t *uisf)
{
  logProcBegin ();

  /* this is run when the filter dialog is first started, */
  /* and after a reset */
  /* all items need to be set, as after a reset, they need to be updated */
  /* sort-by and dance are important, the others can be reset */

  uiplaylistSetKey (uisf->uiplaylist, "");
  if (uisf->playlistname != NULL) {
    uiplaylistSetKey (uisf->uiplaylist, uisf->playlistname);
  }

  uiddSetSelectionByStrKey (uisf->ddsortby,
      songfilterGetSort (uisf->songfilter));
  uidanceSetKey (uisf->uidance, uisf->danceIdx);
  uigenreSetKey (uisf->uigenre,
      songfilterGetNum (uisf->songfilter, SONG_FILTER_GENRE));
  uiEntrySetValue (uisf->wcont [UISF_W_SEARCH],
      (const char *) songfilterGetData (uisf->songfilter, SONG_FILTER_SEARCH));
  uiratingSetValue (uisf->uirating,
      songfilterGetNum (uisf->songfilter, SONG_FILTER_RATING));
  uilevelSetValue (uisf->uilevel,
      songfilterGetNum (uisf->songfilter, SONG_FILTER_LEVEL_LOW));
  uistatusSetValue (uisf->uistatus,
      songfilterGetNum (uisf->songfilter, SONG_FILTER_STATUS));
  uifavoriteSetValue (uisf->uifavorite,
      songfilterGetNum (uisf->songfilter, SONG_FILTER_FAVORITE));
  logProcEnd ("");
}

static void
uisfPlaylistSelect (uisongfilter_t *uisf, const char *str)
{
  pltype_t    pltype;

  logProcBegin ();
  if (str != NULL && *str) {
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
  dataFree (uisf->playlistname);
  uisf->playlistname = mdstrdup (str);
  logProcEnd ("");
}

static void
uisfSortBySelect (uisongfilter_t *uisf, const char *sval)
{
  songfilterSetSort (uisf->songfilter, sval);
  nlistSetStr (uisf->options, SONGSEL_SORT_BY, sval);
  uiddSetSelectionByStrKey (uisf->ddsortby, sval);
}

static void
uisfCreateSortByList (uisongfilter_t *uisf)
{
  slist_t     *sortoptlist;
  ilist_t     *ddlist;
  slistidx_t  iteridx;
  int         count;
  const char  *disp;

  logProcBegin ();

  sortoptlist = sortoptGetList (uisf->sortopt);
  ddlist = ilistAlloc ("sf-sortby-i", LIST_ORDERED);
  ilistSetSize (ddlist, slistGetCount (sortoptlist));

  count = 0;
  slistStartIterator (sortoptlist, &iteridx);
  while ((disp = slistIterateKey (sortoptlist, &iteridx)) != NULL) {
    const char    *sortkey;

    sortkey = slistGetStr (sortoptlist, disp);
    ilistSetStr (ddlist, count, DD_LIST_KEY_STR, sortkey);
    ilistSetStr (ddlist, count, DD_LIST_DISP, disp);
    ++count;
  }
  uisf->ddsortbylist = ddlist;
  logProcEnd ("");
}

static void
uisfGenreSelect (uisongfilter_t *uisf, ssize_t idx)
{
  logProcBegin ();
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_GENRE, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_GENRE);
    }
  }
  logProcEnd ("");
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

  logProcBegin ();

  if (uisf->wcont [UISF_W_DIALOG] != NULL) {
    return;
  }

  szgrp = uiCreateSizeGroupHoriz ();
  szgrpEntry = uiCreateSizeGroupHoriz ();
  szgrpDD = uiCreateSizeGroupHoriz ();
  szgrpSpinText = uiCreateSizeGroupHoriz ();

  uisf->callbacks [UISF_CB_FILTER] = callbackInitI (
      uisfResponseHandler, uisf);
  uisf->wcont [UISF_W_DIALOG] = uiCreateDialog (uisf->window,
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
  uiDialogPackInDialog (uisf->wcont [UISF_W_DIALOG], vbox);

  /* accent color */
  uiutilsAddProfileColorDisplay (vbox, &accent);
  hbox = accent.hbox;
  uiwcontFree (accent.cbox);

  uiwcontFree (hbox);
  /* begin line */

  /* playlist : only available for the music manager */
  /* in this case, the entire hbox will be shown/hidden */
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox);
  uisf->wcont [UISF_W_PLAYLIST] = hbox;

  /* CONTEXT: song selection filter: a filter: select a playlist to work with (music manager) */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uisf->callbacks [UISF_CB_PLAYLIST_SEL] = callbackInitS (
      uisfPlaylistSelectHandler, uisf);
  uisf->uiplaylist = uiplaylistCreate (uisf->wcont [UISF_W_DIALOG], hbox,
      PL_LIST_ALL, NULL, UIPL_PACK_START, UIPL_USE_BLANK);
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

  uisfCreateSortByList (uisf);
  uisf->callbacks [UISF_CB_SORT_BY_SEL] = callbackInitS (
      uisfSortBySelectHandler, uisf);
  /* looks bad if added to the size group */
  uisf->ddsortby = uiddCreate ("sf-sort-by",
      uisf->wcont [UISF_W_DIALOG], hbox, DD_PACK_START,
      uisf->ddsortbylist, DD_LIST_TYPE_STR,
      "", DD_REPLACE_TITLE, uisf->callbacks [UISF_CB_SORT_BY_SEL]);

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

  uiwidgetp = uiEntryInit (20, 100);
  uiWidgetAlignHorizStart (uiwidgetp);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpEntry, uiwidgetp);
  uisf->wcont [UISF_W_SEARCH] = uiwidgetp;

  /* genre */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_GENRE)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_GENRE].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_GENRE] = uiwidgetp;

    uisf->callbacks [UISF_CB_GENRE_SEL] = callbackInitI (
        uisfGenreSelectHandler, uisf);
    uisf->uigenre = uigenreDropDownCreate (hbox, uisf->wcont [UISF_W_DIALOG], true);
    uigenreSetCallback (uisf->uigenre, uisf->callbacks [UISF_CB_GENRE_SEL]);
    /* looks bad if added to the size group */
  }

  /* dance */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCE)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCE].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_DANCE] = uiwidgetp;

    uisf->callbacks [UISF_CB_DANCE_SEL] = callbackInitII (
        uisfDanceSelectHandler, uisf);
    uisf->uidance = uidanceCreate (hbox, uisf->wcont [UISF_W_DIALOG],
        /* CONTEXT: song selection filter: a filter: all dances are selected */
        UIDANCE_ALL_DANCES,  _("All Dances"), UIDANCE_PACK_START, 1);
    uidanceSetCallback (uisf->uidance, uisf->callbacks [UISF_CB_DANCE_SEL]);
    /* adding to the size group makes it look weird */
  }

  /* rating */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCERATING)) {
    uiwcontFree (hbox);
    hbox = uiCreateHorizBox ();
    uiBoxPackStart (vbox, hbox);

    uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
    uiBoxPackStart (hbox, uiwidgetp);
    uiSizeGroupAdd (szgrp, uiwidgetp);
    uisf->labels [UISF_LABEL_DANCE_RATING] = uiwidgetp;

    uisf->uirating = uiratingSpinboxCreate (hbox, UIRATING_ALL);
    uiratingSizeGroupAdd (uisf->uirating, szgrpSpinText);
  }

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

    uisf->wcont [UISF_W_PLAY_STATUS] = uiCreateSwitch (uisf->dfltpbflag);
    uiBoxPackStart (hbox, uisf->wcont [UISF_W_PLAY_STATUS]);
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

  uisf->callbacks [UISF_CB_KEY] = callbackInit (
      uisfKeyHandler, uisf, NULL);
  uiEventSetKeyCallback (uisf->wcont [UISF_W_KEY_HNDLR],
      uisf->wcont [UISF_W_DIALOG],
      uisf->callbacks [UISF_CB_KEY]);

  logProcEnd ("");
}

static bool
uisfResponseHandler (void *udata, int32_t responseid)
{
  uisongfilter_t  *uisf = udata;
  int             x, y, ws;

  logProcBegin ();

  uiWindowGetPosition (uisf->wcont [UISF_W_DIALOG], &x, &y, &ws);
  nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_X, x);
  nlistSetNum (uisf->options, SONGSEL_FILTER_POSITION_Y, y);

  switch (responseid) {
    case RESPONSE_DELETE_WIN: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: del window");
      uisfFreeDialog (uisf);
      break;
    }
    case RESPONSE_CLOSE: {
      logMsg (LOG_DBG, LOG_ACTIONS, "= action: sf: close window");
      uiWidgetHide (uisf->wcont [UISF_W_DIALOG]);
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
      uidanceSetKey (uisf->uidance, uisf->danceIdx);
      if (uisf->danceselcb != NULL) {
        callbackHandlerI (uisf->danceselcb, uisf->danceIdx);
      }
      uisfInitDisplay (uisf);
      if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_STATUS_PLAYABLE)) {
        uiSwitchSetValue (uisf->wcont [UISF_W_PLAY_STATUS], uisf->dfltpbflag);
      }
      break;
    }
  }

  if (responseid != RESPONSE_DELETE_WIN && responseid != RESPONSE_CLOSE) {
    uisfUpdate (uisf);
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
  searchstr = uiEntryGetValue (uisf->wcont [UISF_W_SEARCH]);
  if (searchstr != NULL && strlen (searchstr) > 0) {
    songfilterSetData (uisf->songfilter, SONG_FILTER_SEARCH, (void *) searchstr);
  } else {
    songfilterClear (uisf->songfilter, SONG_FILTER_SEARCH);
  }

  /* dance rating */
  if (songfilterCheckSelection (uisf->songfilter, FILTER_DISP_DANCERATING)) {
    idx = uiratingGetValue (uisf->uirating);
    if (idx >= 0) {
      songfilterSetNum (uisf->songfilter, SONG_FILTER_RATING, idx);
    } else {
      songfilterClear (uisf->songfilter, SONG_FILTER_RATING);
    }
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
    nval = uiSwitchGetValue (uisf->wcont [UISF_W_PLAY_STATUS]);
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
  logProcEnd ("");
}

/* internal routines */

static void
uisfDisableWidgets (uisongfilter_t *uisf)
{
  if (uisf->wcont [UISF_W_DIALOG] == NULL) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetSetState (uisf->labels [i], UIWIDGET_DISABLE);
  }
  uiddSetState (uisf->ddsortby, UIWIDGET_DISABLE);
  uiEntrySetState (uisf->wcont [UISF_W_SEARCH], UIWIDGET_DISABLE);
  uidanceSetState (uisf->uidance, UIWIDGET_DISABLE);
  uigenreSetState (uisf->uigenre, UIWIDGET_DISABLE);
  uiratingSetState (uisf->uirating, UIWIDGET_DISABLE);
  uilevelSetState (uisf->uilevel, UIWIDGET_DISABLE);
  uistatusSetState (uisf->uistatus, UIWIDGET_DISABLE);
  uifavoriteSetState (uisf->uifavorite, UIWIDGET_DISABLE);
  uiWidgetSetState (uisf->wcont [UISF_W_PLAY_STATUS], UIWIDGET_DISABLE);
}

static void
uisfEnableWidgets (uisongfilter_t *uisf)
{
  if (uisf->wcont [UISF_W_DIALOG] == NULL) {
    return;
  }

  for (int i = 0; i < UISF_LABEL_MAX; ++i) {
    uiWidgetSetState (uisf->labels [i], UIWIDGET_ENABLE);
  }
  uiddSetState (uisf->ddsortby, UIWIDGET_ENABLE);
  uiEntrySetState (uisf->wcont [UISF_W_SEARCH], UIWIDGET_ENABLE);
  uidanceSetState (uisf->uidance, UIWIDGET_ENABLE);
  uigenreSetState (uisf->uigenre, UIWIDGET_ENABLE);
  uiratingSetState (uisf->uirating, UIWIDGET_ENABLE);
  uilevelSetState (uisf->uilevel, UIWIDGET_ENABLE);
  uistatusSetState (uisf->uistatus, UIWIDGET_ENABLE);
  uifavoriteSetState (uisf->uifavorite, UIWIDGET_ENABLE);
  uiWidgetSetState (uisf->wcont [UISF_W_PLAY_STATUS], UIWIDGET_ENABLE);
}

static int32_t
uisfPlaylistSelectHandler (void *udata, const char *str)
{
  uisongfilter_t *uisf = udata;

  uisfPlaylistSelect (uisf, str);
  return UICB_CONT;
}

static int32_t
uisfSortBySelectHandler (void *udata, const char *sval)
{
  uisongfilter_t *uisf = udata;

  uisfSortBySelect (uisf, sval);
  return UICB_CONT;
}

static bool
uisfGenreSelectHandler (void *udata, int32_t idx)
{
  uisongfilter_t *uisf = udata;

  uisfGenreSelect (uisf, idx);
  return UICB_CONT;
}

/* count is not used */
static bool
uisfDanceSelectHandler (void *udata, int32_t idx, int32_t count)
{
  uisongfilter_t  *uisf = udata;

  uisf->danceIdx = idx;
  uisfSetDanceIdx (uisf, idx);
  if (uisf->danceselcb != NULL) {
    callbackHandlerI (uisf->danceselcb, idx);
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

    if (uisf->wcont [UISF_W_DIALOG] == NULL) {
      return;
    }

    uiWidgetShowAll (uisf->wcont [UISF_W_PLAYLIST]);
  }

  if (! uisf->showplaylist) {
    songfilterOff (uisf->songfilter, SONG_FILTER_PLAYLIST);

    if (uisf->wcont [UISF_W_DIALOG] == NULL) {
      return;
    }

    uiWidgetHide (uisf->wcont [UISF_W_PLAYLIST]);
  }

  if (songfilterInUse (uisf->songfilter, SONG_FILTER_PLAYLIST)) {
    uisfDisableWidgets (uisf);
  } else {
    uisfEnableWidgets (uisf);
  }
}

static bool
uisfKeyHandler (void *udata)
{
  uisongfilter_t  *uisf = udata;

  if (uisf == NULL) {
    return UICB_CONT;
  }

  if (uiEventIsEnterKey (uisf->wcont [UISF_W_KEY_HNDLR])) {
    uisfResponseHandler (uisf, RESPONSE_APPLY);
  }

  return UICB_CONT;
}

