/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "callback.h"
#include "filemanip.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "pathbld.h"
#include "playlist.h"
#include "tmutil.h"
#include "tagdef.h"
#include "ui.h"
#include "uilevel.h"
#include "uirating.h"
#include "uiselectfile.h"
#include "uiutils.h"
#include "validate.h"

enum {
  MPL_CB_MENU_PL_LOAD,
  MPL_CB_MENU_PL_COPY,
  MPL_CB_MENU_PL_NEW,
  MPL_CB_MENU_PL_DELETE,
  MPL_CB_MAXPLAYTIME,
  MPL_CB_STOPAT,
  MPL_CB_SEL_FILE,
  MPL_CB_MAX,
};

enum {
  MPL_W_RATING_ITEM,
  MPL_W_LOW_LEVEL_ITEM,
  MPL_W_HIGH_LEVEL_ITEM,
  MPL_W_ALLOWED_KEYWORDS,
  MPL_W_PL_TYPE,
  MPL_W_MAX,
};

typedef struct managepl {
  uiwcont_t       *windowp;
  nlist_t         *options;
  uiwcont_t       *errorMsg;
  uimenu_t        *plmenu;
  uiwcont_t       *menuDelete;
  callback_t      *callbacks [MPL_CB_MAX];
  callback_t      *plloadcb;
  char            *ploldname;
  bool            plbackupcreated;
  uientry_t       *plname;
  pltype_t        pltype;
  uispinbox_t     *uimaxplaytime;
  uispinbox_t     *uistopat;
  uiwcont_t       *uistopafter;
  uispinbox_t     *uigap;
  uirating_t      *uirating;
  uiwcont_t       *wcont [MPL_W_MAX];
  uilevel_t       *uilowlevel;
  uilevel_t       *uihighlevel;
  uientry_t       *allowedkeywords;
  managepltree_t  *managepltree;
  playlist_t      *playlist;
  uiswitch_t      *plannswitch;
  bool            changed : 1;
  bool            inload : 1;
} managepl_t;

static bool managePlaylistLoad (void *udata);
static long managePlaylistLoadCB (void *udata, const char *fn);
static bool managePlaylistNewCB (void *udata);
static bool managePlaylistCopy (void *udata);
static void managePlaylistUpdateData (managepl_t *managepl);
static bool managePlaylistDelete (void *udata);
static void manageSetPlaylistName (managepl_t *managepl, const char *nm);
static long managePlaylistValMSCallback (void *udata, const char *txt);
static long managePlaylistValHMCallback (void *udata, const char *txt);
static void managePlaylistUpdatePlaylist (managepl_t *managepl);
static bool managePlaylistCheckChanged (managepl_t *managepl);
static int  managePlaylistAllowedKeywordsChg (uientry_t *e, void *udata);

managepl_t *
managePlaylistAlloc (uiwcont_t *window, nlist_t *options, uiwcont_t *errorMsg)
{
  managepl_t *managepl;

  managepl = mdmalloc (sizeof (managepl_t));
  managepl->menuDelete = NULL;
  managepl->ploldname = NULL;
  managepl->plbackupcreated = false;
  managepl->plmenu = uiMenuAlloc ();
  managepl->plname = uiEntryInit (20, 50);
  managepl->errorMsg = errorMsg;
  managepl->windowp = window;
  managepl->options = options;
  managepl->pltype = PLTYPE_AUTO;
  managepl->uimaxplaytime = uiSpinboxTimeInit (SB_TIME_BASIC);
  managepl->uistopat = uiSpinboxTimeInit (SB_TIME_BASIC);
  managepl->uistopafter = NULL;
  managepl->uigap = uiSpinboxInit ();
  managepl->managepltree = NULL;
  managepl->uirating = NULL;
  managepl->uilowlevel = NULL;
  managepl->uihighlevel = NULL;
  managepl->allowedkeywords = uiEntryInit (15, 50);
  for (int i = 0; i < MPL_W_MAX; ++i) {
    managepl->wcont [i] = NULL;
  }
  managepl->playlist = NULL;
  managepl->changed = false;
  managepl->inload = false;
  managepl->plannswitch = NULL;
  managepl->plloadcb = NULL;
  for (int i = 0; i < MPL_CB_MAX; ++i) {
    managepl->callbacks [i] = NULL;
  }

  managepl->callbacks [MPL_CB_SEL_FILE] =
      callbackInitStr (managePlaylistLoadCB, managepl);

  return managepl;
}

void
managePlaylistFree (managepl_t *managepl)
{
  if (managepl != NULL) {
    uiwcontFree (managepl->uistopafter);
    uiwcontFree (managepl->menuDelete);
    uiMenuFree (managepl->plmenu);
    dataFree (managepl->ploldname);
    if (managepl->managepltree != NULL) {
      managePlaylistTreeFree (managepl->managepltree);
    }
    uiEntryFree (managepl->plname);
    uiEntryFree (managepl->allowedkeywords);
    uiratingFree (managepl->uirating);
    uilevelFree (managepl->uilowlevel);
    uilevelFree (managepl->uihighlevel);
    uiSwitchFree (managepl->plannswitch);
    uiSpinboxFree (managepl->uimaxplaytime);
    uiSpinboxFree (managepl->uistopat);
    uiSpinboxFree (managepl->uigap);
    for (int i = 0; i < MPL_W_MAX; ++i) {
      uiwcontFree (managepl->wcont [i]);
    }
    for (int i = 0; i < MPL_CB_MAX; ++i) {
      callbackFree (managepl->callbacks [i]);
    }
    playlistFree (managepl->playlist);
    mdfree (managepl);
  }
}

void
managePlaylistSetLoadCallback (managepl_t *managepl, callback_t *uicb)
{
  if (managepl == NULL) {
    return;
  }
  managepl->plloadcb = uicb;
}

void
manageBuildUIPlaylist (managepl_t *managepl, uiwcont_t *vboxp)
{
  uiwcont_t          *lcolvbox;
  uiwcont_t          *rcolvbox;
  uiwcont_t          *mainhbox;
  uiwcont_t          *tophbox;
  uiwcont_t          *hbox;
  uiwcont_t          *uiwidgetp;
  uiwcont_t          *szgrp;
  uiwcont_t          *szgrpSpinText;
  uiwcont_t          *szgrpNum;
  uiwcont_t          *szgrpText;

  logProcBegin (LOG_PROC, "manageBuildUIPlaylist");

  szgrp = uiCreateSizeGroupHoriz ();   // labels
  szgrpSpinText = uiCreateSizeGroupHoriz ();  // time widgets + gap widget
  szgrpNum = uiCreateSizeGroupHoriz ();  // numeric widgets
  szgrpText = uiCreateSizeGroupHoriz ();  // text widgets

  uiWidgetSetAllMargins (vboxp, 2);

  tophbox = uiCreateHorizBox ();
  uiWidgetAlignVertStart (tophbox);
  uiBoxPackStart (vboxp, tophbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (tophbox, hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (managepl->plname);
  uiEntrySetValidate (managepl->plname, uiutilsValidatePlaylistName,
      managepl->errorMsg, UIENTRY_IMMEDIATE);
  uiWidgetSetClass (uiEntryGetWidgetContainer (managepl->plname), ACCENT_CLASS);
  uiBoxPackStart (hbox, uiEntryGetWidgetContainer (managepl->plname));

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiWidgetSetMarginStart (hbox, 20);
  uiBoxPackStart (tophbox, hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiwidgetp = uiCreateColonLabel (_("Playlist Type"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiwcontFree (uiwidgetp);

  /* CONTEXT: playlist management: default playlist type */
  uiwidgetp = uiCreateLabel (_("Automatic"));
  uiBoxPackStart (hbox, uiwidgetp);
  managepl->wcont [MPL_W_PL_TYPE] = uiwidgetp;

  mainhbox = uiCreateHorizBox ();
  uiBoxPackStartExpand (vboxp, mainhbox);

  /* left side */
  lcolvbox = uiCreateVertBox ();
  uiBoxPackStart (mainhbox, lcolvbox);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* CONTEXT: playlist management: maximum play time */
  uiwidgetp = uiCreateColonLabel (_("Maximum Play Time"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->callbacks [MPL_CB_MAXPLAYTIME] = callbackInitStr (
      managePlaylistValMSCallback, managepl);
  uiSpinboxTimeCreate (managepl->uimaxplaytime, managepl,
      managepl->callbacks [MPL_CB_MAXPLAYTIME]);
  uiwidgetp = uiSpinboxGetWidgetContainer (managepl->uimaxplaytime);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpSpinText, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* CONTEXT: playlist management: stop at */
  uiwidgetp = uiCreateColonLabel (_("Stop At"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->callbacks [MPL_CB_STOPAT] = callbackInitStr (
      managePlaylistValHMCallback, managepl);
  uiSpinboxTimeCreate (managepl->uistopat, managepl,
      managepl->callbacks [MPL_CB_STOPAT]);
  uiSpinboxSetRange (managepl->uistopat, 0.0, 1440000.0);
  uiSpinboxWrap (managepl->uistopat);
  uiwidgetp = uiSpinboxGetWidgetContainer (managepl->uistopat);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpSpinText, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* CONTEXT: playlist management: stop after */
  uiwidgetp = uiCreateColonLabel (_("Stop After"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwidgetp = uiSpinboxIntCreate ();
  uiSpinboxSet (uiwidgetp, 0.0, 500.0);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpNum, uiwidgetp);
  managepl->uistopafter = uiwidgetp;

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* CONTEXT: playlist management: Gap between songs */
  uiwidgetp = uiCreateColonLabel (_("Gap Between Songs"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiSpinboxDoubleDefaultCreate (managepl->uigap);
  uiSpinboxSetRange (managepl->uigap, -1.0, 60.0);
  uiwidgetp = uiSpinboxGetWidgetContainer (managepl->uigap);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrpText, uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* CONTEXT: playlist management: Play Announcements */
  uiwidgetp = uiCreateColonLabel (_("Play Announcements"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->plannswitch = uiCreateSwitch (0);
  uiBoxPackStart (hbox, uiSwitchGetWidgetContainer (managepl->plannswitch));

  /* automatic and sequenced playlists; keep the widget so these */
  /* can be hidden */

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);

  /* add a blank line between the playlist controls and song selection */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiwcontFree (hbox);
  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);
  managepl->wcont [MPL_W_RATING_ITEM] = hbox;

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->uirating = uiratingSpinboxCreate (hbox, false);
  uiratingSizeGroupAdd (managepl->uirating, szgrpText);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);
  managepl->wcont [MPL_W_LOW_LEVEL_ITEM] = hbox;

  /* CONTEXT: playlist management: Low Dance Level */
  uiwidgetp = uiCreateColonLabel (_("Low Dance Level"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->uilowlevel = uilevelSpinboxCreate (hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, szgrpText);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);
  managepl->wcont [MPL_W_HIGH_LEVEL_ITEM] = hbox;

  /* CONTEXT: playlist management: High Dance Level */
  uiwidgetp = uiCreateColonLabel (_("High Dance Level"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  managepl->uihighlevel = uilevelSpinboxCreate (hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, szgrpText);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (lcolvbox, hbox);
  managepl->wcont [MPL_W_ALLOWED_KEYWORDS] = hbox;

  /* CONTEXT: playlist management: allowed keywords */
  uiwidgetp = uiCreateColonLabel (_("Allowed Keywords"));
  uiBoxPackStart (hbox, uiwidgetp);
  uiSizeGroupAdd (szgrp, uiwidgetp);
  uiwcontFree (uiwidgetp);

  uiEntryCreate (managepl->allowedkeywords);
  uiEntrySetValidate (managepl->allowedkeywords,
      managePlaylistAllowedKeywordsChg, managepl, UIENTRY_IMMEDIATE);
  uiBoxPackStart (hbox, uiEntryGetWidgetContainer (managepl->allowedkeywords));

  /* right side to hold the tree */
  rcolvbox = uiCreateVertBox ();
  uiWidgetSetMarginStart (rcolvbox, 8);
  uiBoxPackStartExpand (mainhbox, rcolvbox);

  managepl->managepltree = managePlaylistTreeAlloc (managepl->errorMsg);
  manageBuildUIPlaylistTree (managepl->managepltree, rcolvbox, tophbox);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managePlaylistNew (managepl, MANAGE_STD);
  managepl->changed = false;

  uiwcontFree (lcolvbox);
  uiwcontFree (rcolvbox);
  uiwcontFree (mainhbox);
  uiwcontFree (tophbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpSpinText);
  uiwcontFree (szgrpNum);
  uiwcontFree (szgrpText);

  logProcEnd (LOG_PROC, "manageBuildUIPlaylist", "");
}

uimenu_t *
managePlaylistMenu (managepl_t *managepl, uiwcont_t *uimenubar)
{
  uiwcont_t   *menu = NULL;
  uiwcont_t   *menuitem = NULL;

  logProcBegin (LOG_PROC, "managePlaylistMenu");
  if (! uiMenuInitialized (managepl->plmenu)) {
    menuitem = uiMenuAddMainItem (uimenubar,
        /* CONTEXT: playlist management: menu selection: playlist: edit menu */
        managepl->plmenu, _("Edit"));
    menu = uiCreateSubMenu (menuitem);
    uiwcontFree (menuitem);

    managepl->callbacks [MPL_CB_MENU_PL_LOAD] = callbackInit (
        managePlaylistLoad, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: load */
    menuitem = uiMenuCreateItem (menu, _("Load"),
        managepl->callbacks [MPL_CB_MENU_PL_LOAD]);
    uiwcontFree (menuitem);

    managepl->callbacks [MPL_CB_MENU_PL_NEW] = callbackInit (
        managePlaylistNewCB, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: new automatic playlist */
    menuitem = uiMenuCreateItem (menu, _("New Automatic Playlist"),
        managepl->callbacks [MPL_CB_MENU_PL_NEW]);
    uiwcontFree (menuitem);

    managepl->callbacks [MPL_CB_MENU_PL_COPY] = callbackInit (
        managePlaylistCopy, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: create copy */
    menuitem = uiMenuCreateItem (menu, _("Create Copy"),
        managepl->callbacks [MPL_CB_MENU_PL_COPY]);
    uiwcontFree (menuitem);

    managepl->callbacks [MPL_CB_MENU_PL_DELETE] = callbackInit (
        managePlaylistDelete, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: delete playlist */
    menuitem = uiMenuCreateItem (menu, _("Delete"),
        managepl->callbacks [MPL_CB_MENU_PL_DELETE]);
    managepl->menuDelete = menuitem;
    /* do not free this menu item here */

    uiMenuSetInitialized (managepl->plmenu);
    uiwcontFree (menu);
  }

  uiMenuDisplay (managepl->plmenu);

  logProcEnd (LOG_PROC, "managePlaylistMenu", "");
  return managepl->plmenu;
}

void
managePlaylistSave (managepl_t *managepl)
{
  char      *name;

  logProcBegin (LOG_PROC, "managePlaylistSave");
  if (managepl->ploldname == NULL) {
    logProcEnd (LOG_PROC, "managePlaylistSave", "no-old-name");
    return;
  }

  name = manageTrimName (uiEntryGetValue (managepl->plname));

  managepl->changed = managePlaylistCheckChanged (managepl);

  /* the playlist has been renamed */
  if (strcmp (managepl->ploldname, name) != 0) {
    playlistRename (managepl->ploldname, name);
    managepl->changed = true;
  }

  if (managepl->changed) {
    pltype_t  pltype;

    manageSetPlaylistName (managepl, name);
    managePlaylistUpdatePlaylist (managepl);
    playlistSave (managepl->playlist, name);
    pltype = playlistGetConfigNum (managepl->playlist, PLAYLIST_TYPE);
    if (managepl->plloadcb != NULL &&
        (pltype == PLTYPE_SONGLIST || pltype == PLTYPE_SEQUENCE)) {
      callbackHandlerStr (managepl->plloadcb, name);
    }
  }
  mdfree (name);
  logProcEnd (LOG_PROC, "managePlaylistSave", "");
}

/* the current playlist may be renamed or deleted. */
/* check for this and if the playlist has */
/* disappeared, reset */
void
managePlaylistLoadCheck (managepl_t *managepl)
{
  char        *name;

  logProcBegin (LOG_PROC, "managePlaylistLoadCheck");
  if (managepl->ploldname == NULL) {
    logProcEnd (LOG_PROC, "managePlaylistLoadCheck", "no-old-name");
    return;
  }

  name = manageTrimName (uiEntryGetValue (managepl->plname));

  if (! playlistExists (name)) {
    managePlaylistNew (managepl, MANAGE_STD);
  }
  mdfree (name);
  logProcEnd (LOG_PROC, "managePlaylistLoadCheck", "");
}

void
managePlaylistLoadFile (managepl_t *managepl, const char *fn, int preloadflag)
{
  playlist_t  *pl;

  if (managepl->inload) {
    return;
  }
  logProcBegin (LOG_PROC, "managePlaylistLoadFile");

  if (preloadflag != MANAGE_PRELOAD_FORCE &&
      managepl->playlist != NULL &&
      strcmp (fn, playlistGetName (managepl->playlist)) == 0) {
    logProcEnd (LOG_PROC, "managePlaylistLoadFile", "already");
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "load playlist file");
  managepl->inload = true;

  if (preloadflag == MANAGE_STD) {
    managePlaylistSave (managepl);
  }

  pl = playlistLoad (fn, NULL);
  if (pl == NULL) {
    managePlaylistNew (managepl, preloadflag);
    managepl->inload = false;
    logProcEnd (LOG_PROC, "managePlaylistLoadFile", "null");
    return;
  }

  if (managepl->menuDelete != NULL) {
    uiWidgetSetState (managepl->menuDelete, UIWIDGET_ENABLE);
    /* CONTEXT: edit sequences: the name for the special playlist used for the 'queue dance' button */
    if (strcmp (playlistGetName (pl), _("QueueDance")) == 0 ||
        strcmp (playlistGetName (pl), "QueueDance") == 0) {
      uiWidgetSetState (managepl->menuDelete, UIWIDGET_DISABLE);
    }
  }

  playlistFree (managepl->playlist);
  managepl->playlist = pl;
  manageSetPlaylistName (managepl, fn);
  managePlaylistUpdateData (managepl);

  if (preloadflag == MANAGE_STD && managepl->plloadcb != NULL) {
    pltype_t    pltype;

    pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);
    if (pltype == PLTYPE_SONGLIST || pltype == PLTYPE_SEQUENCE) {
      callbackHandlerStr (managepl->plloadcb, fn);
    }
  }

  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;
  managepl->inload = false;
  logProcEnd (LOG_PROC, "managePlaylistLoadFile", "");
}

bool
managePlaylistNew (managepl_t *managepl, int preloadflag)
{
  char        tbuff [200];
  playlist_t  *pl = NULL;

  logProcBegin (LOG_PROC, "managePlaylistNew");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new playlist");
  if (preloadflag == MANAGE_STD) {
    managePlaylistSave (managepl);
  }

  /* CONTEXT: playlist management: default name for a new playlist */
  snprintf (tbuff, sizeof (tbuff), _("New Playlist"));
  manageSetPlaylistName (managepl, tbuff);
  managepl->plbackupcreated = false;

  pl = playlistCreate (tbuff, PLTYPE_AUTO, NULL);
  playlistFree (managepl->playlist);
  managepl->playlist = pl;
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;
  managePlaylistUpdateData (managepl);

  logProcEnd (LOG_PROC, "managePlaylistNew", "");
  return UICB_CONT;
}

/* internal routines */

static bool
managePlaylistLoad (void *udata)
{
  managepl_t  *managepl = udata;

  logProcBegin (LOG_PROC, "managePlaylistLoad");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load playlist");
  managePlaylistSave (managepl);
  selectFileDialog (SELFILE_PLAYLIST, managepl->windowp, managepl->options,
      managepl->callbacks [MPL_CB_SEL_FILE]);
  logProcEnd (LOG_PROC, "managePlaylistLoad", "");
  return UICB_CONT;
}

static long
managePlaylistLoadCB (void *udata, const char *fn)
{
  managepl_t  *managepl = udata;

  logProcBegin (LOG_PROC, "managePlaylistLoadCB");
  managePlaylistLoadFile (managepl, fn, MANAGE_STD);
  logProcEnd (LOG_PROC, "managePlaylistLoadCB", "");
  return 0;
}

static bool
managePlaylistNewCB (void *udata)
{
  managepl_t  *managepl = udata;

  logProcBegin (LOG_PROC, "managePlaylistNewCB");
  managePlaylistNew (managepl, MANAGE_STD);
  logProcEnd (LOG_PROC, "managePlaylistNewCB", "");
  return UICB_CONT;
}

static void
managePlaylistUpdateData (managepl_t *managepl)
{
  pltype_t    pltype;
  playlist_t  *pl;

  logProcBegin (LOG_PROC, "managePlaylistUpdateData");
  pl = managepl->playlist;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  managePlaylistTreePrePopulate (managepl->managepltree, pl);
  managePlaylistTreePopulate (managepl->managepltree, pl);

  if (pltype == PLTYPE_SONGLIST) {
    uiWidgetHide (managepl->wcont [MPL_W_RATING_ITEM]);
    uiWidgetHide (managepl->wcont [MPL_W_LOW_LEVEL_ITEM]);
    uiWidgetHide (managepl->wcont [MPL_W_HIGH_LEVEL_ITEM]);
    uiWidgetHide (managepl->wcont [MPL_W_ALLOWED_KEYWORDS]);
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Song List"));
  } else {
    uiWidgetShow (managepl->wcont [MPL_W_RATING_ITEM]);
    uiWidgetShow (managepl->wcont [MPL_W_LOW_LEVEL_ITEM]);
    uiWidgetShow (managepl->wcont [MPL_W_HIGH_LEVEL_ITEM]);
    uiWidgetShow (managepl->wcont [MPL_W_ALLOWED_KEYWORDS]);
    if (pltype == PLTYPE_SEQUENCE) {
      /* CONTEXT: playlist management: type of playlist */
      uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Sequence"));
    }
    if (pltype == PLTYPE_AUTO) {
      /* CONTEXT: playlist management: type of playlist */
      uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Automatic"));
    }
  }

  uiSpinboxTimeSetValue (managepl->uimaxplaytime,
      playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME));
  /* convert the hh:mm value to mm:ss for the spinbox */
  uiSpinboxTimeSetValue (managepl->uistopat,
      playlistGetConfigNum (pl, PLAYLIST_STOP_TIME) / 60);
  uiSpinboxSetValue (managepl->uistopafter,
      playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER));
  uiSpinboxSetValue (uiSpinboxGetWidgetContainer (managepl->uigap),
      (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0);
  uiSwitchSetValue (managepl->plannswitch,
      playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE));
  uiratingSetValue (managepl->uirating,
      playlistGetConfigNum (pl, PLAYLIST_RATING));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH));

  managepl->plbackupcreated = false;
  logProcEnd (LOG_PROC, "managePlaylistUpdateData", "");
}

static bool
managePlaylistCopy (void *udata)
{
  managepl_t  *managepl = udata;
  char        *oname;
  char        newname [200];

  logProcBegin (LOG_PROC, "managePlaylistCopy");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy playlist");
  managePlaylistSave (managepl);

  oname = manageTrimName (uiEntryGetValue (managepl->plname));
  /* CONTEXT: playlist management: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (managepl->errorMsg, oname, newname)) {
    manageSetPlaylistName (managepl, newname);
    managepl->plbackupcreated = false;
    uiSpinboxResetChanged (managepl->uimaxplaytime);
    uiSpinboxResetChanged (managepl->uistopat);
    uiSpinboxResetChanged (managepl->uigap);
    managepl->changed = false;
    if (managepl->plloadcb != NULL) {
      callbackHandlerStr (managepl->plloadcb, newname);
    }
  }
  mdfree (oname);

  logProcEnd (LOG_PROC, "managePlaylistCopy", "");
  return UICB_CONT;
}

static bool
managePlaylistDelete (void *udata)
{
  managepl_t  *managepl = udata;
  char        *oname;

  logProcBegin (LOG_PROC, "managePlaylistDelete");
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: delete playlist");
  oname = manageTrimName (uiEntryGetValue (managepl->plname));
  manageDeletePlaylist (managepl->errorMsg, oname);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;

  managePlaylistNew (managepl, MANAGE_STD);

  mdfree (oname);
  logProcEnd (LOG_PROC, "managePlaylistDelete", "");
  return UICB_CONT;
}

static void
manageSetPlaylistName (managepl_t *managepl, const char *name)
{
  logProcBegin (LOG_PROC, "manageSetPlaylistName");
  dataFree (managepl->ploldname);
  managepl->ploldname = mdstrdup (name);
  uiEntrySetValue (managepl->plname, name);
  logProcEnd (LOG_PROC, "manageSetPlaylistName", "");
}

static long
managePlaylistValMSCallback (void *udata, const char *txt)
{
  managepl_t  *managepl = udata;
  const char  *valstr;
  char        tbuff [200];
  long        value;

  logProcBegin (LOG_PROC, "managePlaylistValMSCallback");
  uiLabelSetText (managepl->errorMsg, "");
  valstr = validate (txt, VAL_MIN_SEC);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->errorMsg, tbuff);
    logProcEnd (LOG_PROC, "managePlaylistValMSCallback", "not-valid");
    return -1;
  }

  value = tmutilStrToMS (txt);
  logProcEnd (LOG_PROC, "managePlaylistValMSCallback", "");
  return value;
}

static long
managePlaylistValHMCallback (void *udata, const char *txt)
{
  managepl_t  *managepl = udata;
  const char  *valstr;
  char        tbuff [200];
  long        value;

  logProcBegin (LOG_PROC, "managePlaylistValHMCallback");
  uiLabelSetText (managepl->errorMsg, "");
  valstr = validate (txt, VAL_HOUR_MIN);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->errorMsg, tbuff);
    logProcEnd (LOG_PROC, "managePlaylistValHMCallback", "not-valid");
    return -1;
  }

  value = tmutilStrToHM (txt);
  logProcEnd (LOG_PROC, "managePlaylistValHMCallback", "");
  return value;
}

static void
managePlaylistUpdatePlaylist (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  double        dval;
  const char    *tstr;

  logProcBegin (LOG_PROC, "managePlaylistUpdatePlaylist");
  pl = managepl->playlist;

  managePlaylistTreeUpdatePlaylist (managepl->managepltree);

  tval = uiSpinboxGetValue (uiSpinboxGetWidgetContainer (managepl->uimaxplaytime));
  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, tval);

  tval = uiSpinboxGetValue (uiSpinboxGetWidgetContainer (managepl->uistopat));
  /* convert the mm:ss value to hh:mm for stop-time */
  tval *= 60;
  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, tval);

  tval = uiSpinboxGetValue (managepl->uistopafter);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, tval);

  dval = uiSpinboxGetValue (uiSpinboxGetWidgetContainer (managepl->uigap));
  playlistSetConfigNum (pl, PLAYLIST_GAP, (long) (dval * 1000.0));

  tval = uiSwitchGetValue (managepl->plannswitch);
  playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE, tval);

  tval = uiratingGetValue (managepl->uirating);
  playlistSetConfigNum (pl, PLAYLIST_RATING, tval);

  tval = uilevelGetValue (managepl->uilowlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, tval);

  tval = uilevelGetValue (managepl->uihighlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, tval);

  tstr = uiEntryGetValue (managepl->allowedkeywords);
  playlistSetConfigList (pl, PLAYLIST_ALLOWED_KEYWORDS, tstr);

  logProcEnd (LOG_PROC, "managePlaylistUpdatePlaylist", "");
}

static bool
managePlaylistCheckChanged (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  double        dval;

  logProcBegin (LOG_PROC, "managePlaylistCheckChanged");
  if (managePlaylistTreeIsChanged (managepl->managepltree)) {
    managepl->changed = true;
  }

  if (uiSpinboxIsChanged (managepl->uimaxplaytime)) {
    managepl->changed = true;
  }

  if (uiSpinboxIsChanged (managepl->uistopat)) {
    managepl->changed = true;
  }

  if (uiSpinboxIsChanged (managepl->uigap)) {
    managepl->changed = true;
  }

  pl = managepl->playlist;

  tval = uiSpinboxGetValue (managepl->uistopafter);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER)) {
    managepl->changed = true;
  }

  dval = uiSpinboxGetValue (uiSpinboxGetWidgetContainer (managepl->uigap));
  if (dval != (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0 ) {
    managepl->changed = true;
  }

  tval = uiSwitchGetValue (managepl->plannswitch);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE)) {
    managepl->changed = true;
  }

  tval = uiratingGetValue (managepl->uirating);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_RATING)) {
    managepl->changed = true;
  }

  tval = uilevelGetValue (managepl->uilowlevel);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW)) {
    managepl->changed = true;
  }

  tval = uilevelGetValue (managepl->uihighlevel);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH)) {
    managepl->changed = true;
  }

  logProcEnd (LOG_PROC, "managePlaylistCheckChanged", "");
  return managepl->changed;
}

static int
managePlaylistAllowedKeywordsChg (uientry_t *e, void *udata)
{
  managepl_t *managepl = udata;

  managepl->changed = true;
  return UIENTRY_OK;
}

