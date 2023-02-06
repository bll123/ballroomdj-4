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

#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "filemanip.h"
#include "log.h"
#include "manageui.h"
#include "mdebug.h"
#include "pathbld.h"
#include "playlist.h"
#include "tmutil.h"
#include "tagdef.h"
#include "ui.h"
#include "callback.h"
#include "uilevel.h"
#include "uirating.h"
#include "uiselectfile.h"
#include "validate.h"

enum {
  MPL_CB_MENU_PL_LOAD,
  MPL_CB_MENU_PL_COPY,
  MPL_CB_MENU_PL_NEW,
  MPL_CB_MENU_PL_DELETE,
  MPL_CB_MAXPLAYTIME,
  MPL_CB_STOPAT,
  MPL_CB_MAX,
};

typedef struct managepl {
  UIWidget        *windowp;
  nlist_t         *options;
  UIWidget        *statusMsg;
  uimenu_t        plmenu;
  UIWidget        menuDelete;
  callback_t      *callbacks [MPL_CB_MAX];
  callback_t      *plloadcb;
  char            *ploldname;
  bool            plbackupcreated;
  uientry_t       *plname;
  pltype_t        pltype;
  uispinbox_t     *uimaxplaytime;
  uispinbox_t     *uistopat;
  UIWidget        uistopafter;
  uispinbox_t     *uigap;
  uirating_t      *uirating;
  UIWidget        uiratingitem;
  uilevel_t       *uilowlevel;
  UIWidget        uilowlevelitem;
  uilevel_t       *uihighlevel;
  UIWidget        uihighlevelitem;
  uientry_t       *allowedkeywords;
  UIWidget        uiallowedkeywordsitem;
  UIWidget        uipltype;
  managepltree_t  *managepltree;
  playlist_t      *playlist;
  uiswitch_t      *plannswitch;
  bool            changed : 1;
  bool            inload : 1;
} managepl_t;

static bool managePlaylistLoad (void *udata);
static void managePlaylistLoadCB (void *udata, const char *fn);
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
managePlaylistAlloc (UIWidget *window, nlist_t *options, UIWidget *statusMsg)
{
  managepl_t *managepl;

  managepl = mdmalloc (sizeof (managepl_t));
  uiutilsUIWidgetInit (&managepl->uipltype);
  uiutilsUIWidgetInit (&managepl->menuDelete);
  managepl->ploldname = NULL;
  managepl->plbackupcreated = false;
  uiMenuInit (&managepl->plmenu);
  managepl->plname = uiEntryInit (20, 50);
  managepl->statusMsg = statusMsg;
  managepl->windowp = window;
  managepl->options = options;
  managepl->pltype = PLTYPE_AUTO;
  managepl->uimaxplaytime = uiSpinboxTimeInit (SB_TIME_BASIC);
  managepl->uistopat = uiSpinboxTimeInit (SB_TIME_BASIC);
  uiutilsUIWidgetInit (&managepl->uistopafter);
  managepl->uigap = uiSpinboxInit ();
  managepl->managepltree = NULL;
  managepl->uirating = NULL;
  managepl->uilowlevel = NULL;
  managepl->uihighlevel = NULL;
  uiutilsUIWidgetInit (&managepl->uiratingitem);
  uiutilsUIWidgetInit (&managepl->uilowlevelitem);
  uiutilsUIWidgetInit (&managepl->uihighlevelitem);
  managepl->allowedkeywords = uiEntryInit (15, 50);
  uiutilsUIWidgetInit (&managepl->uiallowedkeywordsitem);
  managepl->playlist = NULL;
  managepl->changed = false;
  managepl->inload = false;
  managepl->plannswitch = NULL;
  managepl->plloadcb = NULL;
  for (int i = 0; i < MPL_CB_MAX; ++i) {
    managepl->callbacks [i] = NULL;
  }

  return managepl;
}

void
managePlaylistFree (managepl_t *managepl)
{
  if (managepl != NULL) {
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
manageBuildUIPlaylist (managepl_t *managepl, UIWidget *vboxp)
{
  UIWidget            lcol;
  UIWidget            rcol;
  UIWidget            mainhbox;
  UIWidget            tophbox;
  UIWidget            hbox;
  UIWidget            uiwidget;
  UIWidget            *uiwidgetp;
  UIWidget            sg;
  UIWidget            sgA;
  UIWidget            sgB;
  UIWidget            sgC;

  logProcBegin (LOG_PROC, "manageBuildUIPlaylist");
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);
  uiCreateSizeGroupHoriz (&sg);   // labels
  uiCreateSizeGroupHoriz (&sgA);  // time widgets + gap widget
  uiCreateSizeGroupHoriz (&sgB);  // numeric widgets
  uiCreateSizeGroupHoriz (&sgC);  // text widgets

  uiWidgetSetAllMargins (vboxp, 2);

  uiCreateHorizBox (&tophbox);
  uiWidgetAlignVertStart (&tophbox);
  uiBoxPackStart (vboxp, &tophbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&tophbox, &hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiCreateColonLabel (&uiwidget, _("Playlist"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiEntryCreate (managepl->plname);
  uiEntrySetValidate (managepl->plname, manageValidateName,
      managepl->statusMsg, UIENTRY_IMMEDIATE);
  uiWidgetSetClass (uiEntryGetUIWidget (managepl->plname), ACCENT_CLASS);
  uiBoxPackStart (&hbox, uiEntryGetUIWidget (managepl->plname));

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginStart (&hbox, 20);
  uiBoxPackStart (&tophbox, &hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiCreateColonLabel (&uiwidget, _("Playlist Type"));
  uiBoxPackStart (&hbox, &uiwidget);

  /* CONTEXT: playlist management: default playlist type */
  uiCreateLabel (&uiwidget, _("Automatic"));
  uiWidgetSetClass (&uiwidget, ACCENT_CLASS);
  uiBoxPackStart (&hbox, &uiwidget);
  uiutilsUIWidgetCopy (&managepl->uipltype, &uiwidget);

  uiCreateHorizBox (&mainhbox);
  uiBoxPackStartExpand (vboxp, &mainhbox);

  /* left side */
  uiCreateVertBox (&lcol);
  uiBoxPackStart (&mainhbox, &lcol);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: maximum play time */
  uiCreateColonLabel (&uiwidget, _("Maximum Play Time"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->callbacks [MPL_CB_MAXPLAYTIME] = callbackInitStr (
      managePlaylistValMSCallback, managepl);
  uiSpinboxTimeCreate (managepl->uimaxplaytime, managepl,
      managepl->callbacks [MPL_CB_MAXPLAYTIME]);
  uiwidgetp = uiSpinboxGetUIWidget (managepl->uimaxplaytime);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgA, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: stop at */
  uiCreateColonLabel (&uiwidget, _("Stop At"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->callbacks [MPL_CB_STOPAT] = callbackInitStr (
      managePlaylistValHMCallback, managepl);
  uiSpinboxTimeCreate (managepl->uistopat, managepl,
      managepl->callbacks [MPL_CB_STOPAT]);
  uiSpinboxSetRange (managepl->uistopat, 0.0, 1440000.0);
  uiSpinboxWrap (managepl->uistopat);
  uiwidgetp = uiSpinboxGetUIWidget (managepl->uistopat);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgA, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: stop after */
  uiCreateColonLabel (&uiwidget, _("Stop After"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiSpinboxIntCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, 0.0, 500.0);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgB, &uiwidget);
  uiutilsUIWidgetCopy (&managepl->uistopafter, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: Gap between songs */
  uiCreateColonLabel (&uiwidget, _("Gap Between Songs"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiSpinboxDoubleDefaultCreate (managepl->uigap);
  uiSpinboxSetRange (managepl->uigap, -1.0, 60.0);
  uiwidgetp = uiSpinboxGetUIWidget (managepl->uigap);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgC, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: Play Announcements */
  uiCreateColonLabel (&uiwidget, _("Play Announcements"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->plannswitch = uiCreateSwitch (0);
  uiBoxPackStart (&hbox, uiSwitchGetUIWidget (managepl->plannswitch));

  /* automatic and sequenced playlists; keep the widget so these */
  /* can be hidden */

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* add a blank line between the playlist controls and song selection */
  uiCreateLabel (&uiwidget, "");
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uiratingitem, &hbox);

  uiCreateColonLabel (&uiwidget, tagdefs [TAG_DANCERATING].displayname);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uirating = uiratingSpinboxCreate (&hbox, false);
  uiratingSizeGroupAdd (managepl->uirating, &sgC);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uilowlevelitem, &hbox);

  /* CONTEXT: playlist management: Low Dance Level */
  uiCreateColonLabel (&uiwidget, _("Low Dance Level"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uilowlevel = uilevelSpinboxCreate (&hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, &sgC);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uihighlevelitem, &hbox);

  /* CONTEXT: playlist management: High Dance Level */
  uiCreateColonLabel (&uiwidget, _("High Dance Level"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uihighlevel = uilevelSpinboxCreate (&hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, &sgC);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uiallowedkeywordsitem, &hbox);

  /* CONTEXT: playlist management: allowed keywords */
  uiCreateColonLabel (&uiwidget, _("Allowed Keywords"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiEntryCreate (managepl->allowedkeywords);
  uiEntrySetValidate (managepl->allowedkeywords,
      managePlaylistAllowedKeywordsChg, managepl, UIENTRY_IMMEDIATE);
  uiBoxPackStart (&hbox, uiEntryGetUIWidget (managepl->allowedkeywords));

  /* right side to hold the tree */
  uiCreateVertBox (&rcol);
  uiWidgetSetMarginStart (&rcol, 8);
  uiBoxPackStartExpand (&mainhbox, &rcol);

  managepl->managepltree = managePlaylistTreeAlloc (managepl->statusMsg);
  manageBuildUIPlaylistTree (managepl->managepltree, &rcol, &tophbox);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managePlaylistNew (managepl, MANAGE_STD);
  managepl->changed = false;
  logProcEnd (LOG_PROC, "manageBuildUIPlaylist", "");
}

uimenu_t *
managePlaylistMenu (managepl_t *managepl, UIWidget *uimenubar)
{
  UIWidget  menu;
  UIWidget  menuitem;

  logProcBegin (LOG_PROC, "managePlaylistMenu");
  if (! managepl->plmenu.initialized) {
    uiMenuAddMainItem (uimenubar, &menuitem,
        /* CONTEXT: playlist management: menu selection: playlist: edit menu */
        &managepl->plmenu, _("Edit"));

    uiCreateSubMenu (&menuitem, &menu);

    managepl->callbacks [MPL_CB_MENU_PL_LOAD] = callbackInit (
        managePlaylistLoad, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: load */
    uiMenuCreateItem (&menu, &menuitem, _("Load"),
        managepl->callbacks [MPL_CB_MENU_PL_LOAD]);

    managepl->callbacks [MPL_CB_MENU_PL_NEW] = callbackInit (
        managePlaylistNewCB, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: new automatic playlist */
    uiMenuCreateItem (&menu, &menuitem, _("New Automatic Playlist"),
        managepl->callbacks [MPL_CB_MENU_PL_NEW]);

    managepl->callbacks [MPL_CB_MENU_PL_COPY] = callbackInit (
        managePlaylistCopy, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: create copy */
    uiMenuCreateItem (&menu, &menuitem, _("Create Copy"),
        managepl->callbacks [MPL_CB_MENU_PL_COPY]);

    managepl->callbacks [MPL_CB_MENU_PL_DELETE] = callbackInit (
        managePlaylistDelete, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: delete playlist */
    uiMenuCreateItem (&menu, &menuitem, _("Delete"),
        managepl->callbacks [MPL_CB_MENU_PL_DELETE]);
    uiutilsUIWidgetCopy (&managepl->menuDelete, &menuitem);

    managepl->plmenu.initialized = true;
  }

  uiMenuDisplay (&managepl->plmenu);
  logProcEnd (LOG_PROC, "managePlaylistMenu", "");
  return &managepl->plmenu;
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

  if (managepl->playlist != NULL &&
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

  if (uiutilsUIWidgetSet (&managepl->menuDelete)) {
    uiWidgetEnable (&managepl->menuDelete);
    /* CONTEXT: edit sequences: the name for the special playlist used for the 'queue dance' button */
    if (strcmp (playlistGetName (pl), _("QueueDance")) == 0) {
      uiWidgetDisable (&managepl->menuDelete);
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
      managepl, managePlaylistLoadCB);
  logProcEnd (LOG_PROC, "managePlaylistLoad", "");
  return UICB_CONT;
}

static void
managePlaylistLoadCB (void *udata, const char *fn)
{
  managepl_t  *managepl = udata;

  logProcBegin (LOG_PROC, "managePlaylistLoadCB");
  managePlaylistLoadFile (managepl, fn, MANAGE_STD);
  logProcEnd (LOG_PROC, "managePlaylistLoadCB", "");
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
    uiWidgetHide (&managepl->uiratingitem);
    uiWidgetHide (&managepl->uilowlevelitem);
    uiWidgetHide (&managepl->uihighlevelitem);
    uiWidgetHide (&managepl->uiallowedkeywordsitem);
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (&managepl->uipltype, _("Song List"));
  } else {
    uiWidgetShow (&managepl->uiratingitem);
    uiWidgetShow (&managepl->uilowlevelitem);
    uiWidgetShow (&managepl->uihighlevelitem);
    uiWidgetShow (&managepl->uiallowedkeywordsitem);
    if (pltype == PLTYPE_SEQUENCE) {
      /* CONTEXT: playlist management: type of playlist */
      uiLabelSetText (&managepl->uipltype, _("Sequence"));
    }
    if (pltype == PLTYPE_AUTO) {
      /* CONTEXT: playlist management: type of playlist */
      uiLabelSetText (&managepl->uipltype, _("Automatic"));
    }
  }

  uiSpinboxTimeSetValue (managepl->uimaxplaytime,
      playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME));
  /* convert the hh:mm value to mm:ss for the spinbox */
  uiSpinboxTimeSetValue (managepl->uistopat,
      playlistGetConfigNum (pl, PLAYLIST_STOP_TIME) / 60);
  uiSpinboxSetValue (&managepl->uistopafter,
      playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER));
  uiSpinboxSetValue (uiSpinboxGetUIWidget (managepl->uigap),
      (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0);
  uiratingSetValue (managepl->uirating,
      playlistGetConfigNum (pl, PLAYLIST_RATING));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH));
  uiSwitchSetValue (managepl->plannswitch,
      playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE));

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
  if (manageCreatePlaylistCopy (managepl->statusMsg, oname, newname)) {
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
  manageDeletePlaylist (managepl->statusMsg, oname);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;

  managePlaylistNew (managepl, MANAGE_STD);
  managePlaylistUpdateData (managepl);

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
  uiLabelSetText (managepl->statusMsg, "");
  valstr = validate (txt, VAL_MIN_SEC);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->statusMsg, tbuff);
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
  uiLabelSetText (managepl->statusMsg, "");
  valstr = validate (txt, VAL_HOUR_MIN);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->statusMsg, tbuff);
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

  tval = uiSpinboxGetValue (uiSpinboxGetUIWidget (managepl->uimaxplaytime));
  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, tval);

  tval = uiSpinboxGetValue (uiSpinboxGetUIWidget (managepl->uistopat));
  /* convert the mm:ss value to hh:mm for stop-time */
  tval *= 60;
  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, tval);

  tval = uiSpinboxGetValue (&managepl->uistopafter);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, tval);

  dval = uiSpinboxGetValue (uiSpinboxGetUIWidget (managepl->uigap));
  playlistSetConfigNum (pl, PLAYLIST_GAP, (long) (dval * 1000.0));

  tval = uiratingGetValue (managepl->uirating);
  playlistSetConfigNum (pl, PLAYLIST_RATING, tval);

  tval = uilevelGetValue (managepl->uilowlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, tval);

  tval = uilevelGetValue (managepl->uihighlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, tval);

  tstr = uiEntryGetValue (managepl->allowedkeywords);
  playlistSetConfigList (pl, PLAYLIST_ALLOWED_KEYWORDS, tstr);

  tval = uiSwitchGetValue (managepl->plannswitch);
  playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE, tval);
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

  tval = uiSpinboxGetValue (&managepl->uistopafter);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER)) {
    managepl->changed = true;
  }

  dval = uiSpinboxGetValue (uiSpinboxGetUIWidget (managepl->uigap));
  if (dval != (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0 ) {
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

