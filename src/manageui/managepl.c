/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "podcast.h"
#include "tmutil.h"
#include "tagdef.h"
#include "ui.h"
#include "uihnb.h"
#include "uilevel.h"
#include "uirating.h"
#include "uisbnum.h"
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
  MPL_CB_RETAIN,
  MPL_CB_MAX,
};

enum {
  MPL_W_MENUITEM_DELETE,
  MPL_W_MENU_PL,
  MPL_W_PL_TYPE,
  MPL_W_PL_NAME,
  MPL_W_STD_VBOX,
  MPL_W_PLAY_ANN,
  MPL_W_AUTO_SEQ_VBOX,
  MPL_W_ALLOWED_KEYWORDS,
  MPL_W_TAGS,
  MPL_W_PODCAST_VBOX,
  MPL_W_URI,
  MPL_W_TITLE,
  MPL_W_MAX,
};

enum {
  MPL_SB_GAP,
  MPL_SB_STOP_AFTER,
  MPL_SB_TAG_WEIGHT,
  MPL_SB_RETAIN,
  MPL_SB_MAX_PLAY_TIME,
  MPL_SB_STOP_AT,
  MPL_SB_MAX,
};

enum {
  MPL_TAB_SETTINGS,
  MPL_TAB_DANCES,
};

typedef struct managepl {
  manageinfo_t    *minfo;
  uihnb_t         *hnb;
  callback_t      *callbacks [MPL_CB_MAX];
  callback_t      *plloadcb;
  char            *ploldname;
  bool            plbackupcreated;
  pltype_t        pltype;
  uirating_t      *uirating;
  uiwcont_t       *wcont [MPL_W_MAX];
  uisbnum_t       *sbnum [MPL_SB_MAX];
  uilevel_t       *uilowlevel;
  uilevel_t       *uihighlevel;
  mpldance_t      *mpldnc;
  playlist_t      *playlist;
  bool            changed;
  bool            inload;
} managepl_t;

static bool managePlaylistLoad (void *udata);
static int32_t managePlaylistLoadCB (void *udata, const char *fn);
static bool managePlaylistNewAutoCB (void *udata);
static bool managePlaylistNewPodcastCB (void *udata);
static bool managePlaylistCopy (void *udata);
static void managePlaylistUpdateData (managepl_t *managepl);
static bool managePlaylistDelete (void *udata);
static void manageSetPlaylistName (managepl_t *managepl, const char *nm);
static bool managePlaylistValMaxPlayTimeCallback (void *udata);
static bool managePlaylistValStopAtCallback (void *udata);
static bool managePlaylistValidateCallback (managepl_t *managepl, int idx);
static void managePlaylistUpdatePlaylist (managepl_t *managepl);
static bool managePlaylistCheckChanged (managepl_t *managepl);
static int  managePlaylistTextEntryChg (uiwcont_t *e, const char *label, void *udata);
static void manageResetChanged (managepl_t *managepl);

managepl_t *
managePlaylistAlloc (manageinfo_t *minfo)
{
  managepl_t *managepl;

  managepl = mdmalloc (sizeof (managepl_t));
  managepl->minfo = minfo;
  for (int i = 0; i < MPL_W_MAX; ++i) {
    managepl->wcont [i] = NULL;
  }
  managepl->wcont [MPL_W_MENUITEM_DELETE] = NULL;
  managepl->ploldname = NULL;
  managepl->plbackupcreated = false;
  managepl->wcont [MPL_W_MENU_PL] = uiMenuAlloc ();
  managepl->pltype = PLTYPE_AUTO;
  managepl->mpldnc = NULL;
  managepl->uirating = NULL;
  managepl->uilowlevel = NULL;
  managepl->uihighlevel = NULL;
  managepl->playlist = NULL;
  managepl->changed = false;
  managepl->inload = false;
  managepl->plloadcb = NULL;
  for (int i = 0; i < MPL_CB_MAX; ++i) {
    managepl->callbacks [i] = NULL;
  }

  managepl->callbacks [MPL_CB_SEL_FILE] =
      callbackInitS (managePlaylistLoadCB, managepl);

  return managepl;
}

void
managePlaylistFree (managepl_t *managepl)
{
  if (managepl == NULL) {
    return;
  }

  dataFree (managepl->ploldname);
  if (managepl->mpldnc != NULL) {
    manageplDanceFree (managepl->mpldnc);
  }
  uiratingFree (managepl->uirating);
  uilevelFree (managepl->uilowlevel);
  uilevelFree (managepl->uihighlevel);
  for (int i = 0; i < MPL_W_MAX; ++i) {
    uiwcontFree (managepl->wcont [i]);
  }
  for (int i = 0; i < MPL_SB_MAX; ++i) {
    uisbnumFree (managepl->sbnum [i]);
  }
  for (int i = 0; i < MPL_CB_MAX; ++i) {
    callbackFree (managepl->callbacks [i]);
  }
  uihnbFree (managepl->hnb);
  playlistFree (managepl->playlist);
  mdfree (managepl);
}

void
managePlaylistProcess (managepl_t *managepl)
{
  for (int i = 0; i < MPL_SB_MAX; ++i) {
    uisbnumCheck (managepl->sbnum [i]);
  }
  manageplDanceProcess (managepl->mpldnc);
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
  uiwcont_t         *mvbox;
  uiwcont_t         *vbox;
  uiwcont_t         *tophbox;
  uiwcont_t         *hbox;
  uiwcont_t         *uiwidgetp;
  uiwcont_t         *szgrp;
  uiwcont_t         *szgrpSpin;
  uiwcont_t         *szgrpText;
  uisbnum_t         *sb;
  const char        *tlabel;

  logProcBegin ();

  szgrp = uiCreateSizeGroupHoriz ();          // labels
  szgrpSpin = uiCreateSizeGroupHoriz ();  // time widgets + gap widget
  szgrpText = uiCreateSizeGroupHoriz ();      // text widgets

  tophbox = uiCreateHorizBox ();
  uiBoxPackStart (vboxp, tophbox, WCONT_FREE);
  uiWidgetAlignVertStart (tophbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (tophbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: label for playlist name */
  uiwidgetp = uiCreateColonLabel (_("Playlist"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  uiwidgetp = uiEntryInit (30, MAX_PL_NM_LEN);
  uiWidgetSetClass (uiwidgetp, ACCENT_CLASS);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  managepl->wcont [MPL_W_PL_NAME] = uiwidgetp;
  /* CONTEXT: playlist management: label for playlist name */
  uiEntrySetValidate (uiwidgetp, _("Playlist"),
      uiutilsValidatePlaylistNameClr,
      managepl->minfo->errorMsg, UIENTRY_IMMEDIATE);

  uiBoxPostProcess (hbox);

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (tophbox, hbox, WCONT_FREE);
  uiWidgetSetMarginStart (hbox, 20);

  /* CONTEXT: playlist management: label for playlist name */
  uiwidgetp = uiCreateColonLabel (_("Playlist Type"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);

  /* CONTEXT: playlist management: default playlist type */
  uiwidgetp = uiCreateLabel (_("Automatic"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiWidgetSetMarginStart (uiwidgetp, 3);
  managepl->wcont [MPL_W_PL_TYPE] = uiwidgetp;

  uiBoxPostProcess (hbox);

  managepl->hnb = uihnbCreate (vboxp, "mng-pl");

  /* settings */

  mvbox = uiCreateVertBox ();
  /* CONTEXT: playlist management: notebook tab title: settings */
  uihnbAppendPage (managepl->hnb, mvbox, _("Settings"),
      NULL, NULL, MPL_TAB_SETTINGS);
  uiWidgetSetMarginTop (mvbox, 4);
  uiWidgetSetMarginStart (mvbox, 4);
  uiWidgetSetMarginEnd (mvbox, 4);

  /* standard settings for most playlists, but not podcasts */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (mvbox, vbox, WCONT_KEEP);
  managepl->wcont [MPL_W_STD_VBOX] = vbox;

  /* new line : max-play-time */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: maximum play time */
  tlabel = _("Maximum Play Time");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  managepl->callbacks [MPL_CB_MAXPLAYTIME] = callbackInit (
      managePlaylistValMaxPlayTimeCallback, managepl, NULL);
  sb = uisbnumCreate (hbox, tlabel, 2);
  /* max 2 hours */
  uisbnumSetTime (sb, 0.0, 3600.0 * 2.0, SBNUM_TIME_MS);
  uisbnumSizeGroupAdd (sb, szgrpSpin);
  uisbnumSetChangeCallback (sb, managepl->callbacks [MPL_CB_MAXPLAYTIME]);
  managepl->sbnum [MPL_SB_MAX_PLAY_TIME] = sb;

  uiBoxPostProcess (hbox);

  /* new line : stop-at */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: stop at */
  tlabel = _("Stop At");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  managepl->callbacks [MPL_CB_STOPAT] = callbackInit (
      managePlaylistValStopAtCallback, managepl, NULL);
  sb = uisbnumCreate (hbox, tlabel, 2);
  /* max 24 hours */
  uisbnumSetTime (sb, 0.0, 3600.0 * 24.0, SBNUM_TIME_HM);
  uisbnumSizeGroupAdd (sb, szgrpSpin);
  managepl->sbnum [MPL_SB_STOP_AT] = sb;
  uisbnumSetChangeCallback (sb, managepl->callbacks [MPL_CB_STOPAT]);

  uiBoxPostProcess (hbox);

  /* new line : stop-after */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: stop after */
  tlabel = _("Stop After");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  sb = uisbnumCreate (hbox, tlabel, 2);
  uisbnumSetLimits (sb, 0.0, 500.0, 0);
  uisbnumSizeGroupAdd (sb, szgrpSpin);
  managepl->sbnum [MPL_SB_STOP_AFTER] = sb;

  uiBoxPostProcess (hbox);

  /* new line: gap */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: Gap between songs */
  tlabel = _("Gap Between Songs");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  sb = uisbnumCreate (hbox, tlabel, 2);
  uisbnumSetLimits (sb, 0.0, 60.0, 1);
  uisbnumSetIncrements (sb, 0.1, 5.0);
  /* must come after limits are set */
  uisbnumSetType (sb, SBNUM_NUM_DEFAULT);
  uisbnumSizeGroupAdd (sb, szgrpText);
  managepl->sbnum [MPL_SB_GAP] = sb;

  uiBoxPostProcess (hbox);

  /* new line : play-announcements */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: Play Announcements */
  uiwidgetp = uiCreateColonLabel (_("Play Announcements"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiwidgetp = uiCreateSwitch (0);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  managepl->wcont [MPL_W_PLAY_ANN] = uiwidgetp;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);

  /* automatic and sequenced playlists; keep the widget so these */
  /* can be hidden */

  /* new line : blank line */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (mvbox, vbox, WCONT_KEEP);
  managepl->wcont [MPL_W_AUTO_SEQ_VBOX] = vbox;

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* add a blank line between the playlist controls and song selection */
  uiwidgetp = uiCreateLabel ("");
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiBoxPostProcess (hbox);

  /* new line : dance-rating */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  uiwidgetp = uiCreateColonLabel (tagdefs [TAG_DANCERATING].displayname);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  managepl->uirating = uiratingSpinboxCreate (hbox, UIRATING_NORM);
  uiratingSizeGroupAdd (managepl->uirating, szgrpText);

  uiBoxPostProcess (hbox);

  /* new line : low dance-level */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: Low Dance Level */
  uiwidgetp = uiCreateColonLabel (_("Low Dance Level"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  managepl->uilowlevel = uilevelSpinboxCreate (hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, szgrpText);

  uiBoxPostProcess (hbox);

  /* new line : high dance-level */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: High Dance Level */
  uiwidgetp = uiCreateColonLabel (_("High Dance Level"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  managepl->uihighlevel = uilevelSpinboxCreate (hbox, false);
  uilevelSizeGroupAdd (managepl->uilowlevel, szgrpText);

  uiBoxPostProcess (hbox);

  /* new line : allowed-keywords */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: allowed keywords */
  uiwidgetp = uiCreateColonLabel (_("Allowed Keywords"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiwidgetp = uiEntryInit (15, 80);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiEntrySetValidate (uiwidgetp, "",
      managePlaylistTextEntryChg, managepl, UIENTRY_IMMEDIATE);
  managepl->wcont [MPL_W_ALLOWED_KEYWORDS] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* new line : tags, tag weight */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: tags */
  uiwidgetp = uiCreateColonLabel (_("Tags"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiwidgetp = uiEntryInit (15, 100);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiEntrySetValidate (uiwidgetp, "",
      managePlaylistTextEntryChg, managepl, UIENTRY_IMMEDIATE);
  managepl->wcont [MPL_W_TAGS] = uiwidgetp;

  /* CONTEXT: playlist management: tag weight */
  tlabel = _("Weight");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiWidgetSetMarginStart (uiwidgetp, 2);

  sb = uisbnumCreate (hbox, tlabel, 2);
  uisbnumSetLimits (sb, 5.0, 100.0, 0);
  uisbnumSetValue (sb, BDJ4_DFLT_TAG_WEIGHT);
  managepl->sbnum [MPL_SB_TAG_WEIGHT] = sb;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);

  /* settings for podcasts */

  vbox = uiCreateVertBox ();
  uiBoxPackStart (mvbox, vbox, WCONT_KEEP);
  managepl->wcont [MPL_W_PODCAST_VBOX] = vbox;

  /* new line : uri */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: podcast URL */
  uiwidgetp = uiCreateColonLabel (_("URL"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiwidgetp = uiEntryInit (40, 500);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiEntrySetValidate (uiwidgetp, "",
      managePlaylistTextEntryChg, managepl, UIENTRY_IMMEDIATE);
  managepl->wcont [MPL_W_URI] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* new line : title */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: podcast title */
  uiwidgetp = uiCreateColonLabel (_("Title"));
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  uiwidgetp = uiEntryInit (50, 300);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_KEEP);
  uiEntrySetValidate (uiwidgetp, "",
      managePlaylistTextEntryChg, managepl, UIENTRY_IMMEDIATE);
  managepl->wcont [MPL_W_TITLE] = uiwidgetp;

  uiBoxPostProcess (hbox);

  /* new line : retain */

  hbox = uiCreateHorizBox ();
  uiBoxPackStart (vbox, hbox, WCONT_FREE);

  /* CONTEXT: playlist management: how many days to keep the podcast */
  tlabel = _("Days to Keep");
  uiwidgetp = uiCreateColonLabel (tlabel);
  uiBoxPackStart (hbox, uiwidgetp, WCONT_FREE);
  uiSizeGroupAdd (szgrp, uiwidgetp);

  sb = uisbnumCreate (hbox, tlabel, 2);
  uisbnumSetLimits (sb, 0.0, 720.0, 0);
  uisbnumSizeGroupAdd (sb, szgrpSpin);
  managepl->sbnum [MPL_SB_RETAIN] = sb;

  uiBoxPostProcess (hbox);
  uiBoxPostProcess (vbox);
  uiBoxPostProcess (mvbox);

  /* dance settings : holds the list of dances with settings */

  vbox = uiCreateVertBox ();
  /* CONTEXT: playlist management: notebook tab title: dances */
  uihnbAppendPage (managepl->hnb, vbox, _("Dances"),
      NULL, NULL, MPL_TAB_DANCES);
  uiWidgetSetMarginTop (vbox, 4);
  uiWidgetSetMarginStart (vbox, 4);
  uiWidgetSetMarginEnd (vbox, 4);

  managepl->mpldnc = manageplDanceAlloc (managepl->minfo);
  manageplDanceBuildUI (managepl->mpldnc, vbox);

  uiBoxPostProcess (vbox);

  uiBoxPostProcess (tophbox);
  uiwcontFree (szgrp);
  uiwcontFree (szgrpSpin);
  uiwcontFree (szgrpText);

  uihnbPostProcess (managepl->hnb);

  managePlaylistNew (managepl, MANAGE_STD, PLTYPE_AUTO);
  manageResetChanged (managepl);

  logProcEnd ("");
}

uiwcont_t *
managePlaylistMenu (managepl_t *managepl, uiwcont_t *uimenubar)
{
  uiwcont_t   *menu = NULL;
  uiwcont_t   *menuitem = NULL;

  logProcBegin ();
  if (uiMenuIsInitialized (managepl->wcont [MPL_W_MENU_PL])) {
    uiMenuDisplay (managepl->wcont [MPL_W_MENU_PL]);
    logProcEnd ("menu-is-init");
    return managepl->wcont [MPL_W_MENU_PL];
  }

  menuitem = uiMenuAddMainItem (uimenubar,
      /* CONTEXT: playlist management: menu selection: playlist: edit menu */
      managepl->wcont [MPL_W_MENU_PL], _("Edit"));
  menu = uiCreateSubMenu (menuitem);
  uiwcontFree (menuitem);

  managepl->callbacks [MPL_CB_MENU_PL_LOAD] = callbackInit (
      managePlaylistLoad, managepl, NULL);
  /* CONTEXT: playlist management: menu selection: playlist: edit menu: load */
  menuitem = uiMenuCreateItem (menu, _("Load"),
      managepl->callbacks [MPL_CB_MENU_PL_LOAD]);
  uiwcontFree (menuitem);

  managepl->callbacks [MPL_CB_MENU_PL_NEW] = callbackInit (
      managePlaylistNewAutoCB, managepl, NULL);
  /* CONTEXT: playlist management: menu selection: playlist: edit menu: new automatic playlist */
  menuitem = uiMenuCreateItem (menu, _("New Automatic Playlist"),
      managepl->callbacks [MPL_CB_MENU_PL_NEW]);
  uiwcontFree (menuitem);

  managepl->callbacks [MPL_CB_MENU_PL_NEW] = callbackInit (
      managePlaylistNewPodcastCB, managepl, NULL);
  /* CONTEXT: playlist management: menu selection: playlist: edit menu: new podcast */
  menuitem = uiMenuCreateItem (menu, _("New Podcast"),
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
  managepl->wcont [MPL_W_MENUITEM_DELETE] = menuitem;
  /* do not free this menu item here */

  uiMenuSetInitialized (managepl->wcont [MPL_W_MENU_PL]);
  uiwcontFree (menu);

  uiMenuDisplay (managepl->wcont [MPL_W_MENU_PL]);

  logProcEnd ("");
  return managepl->wcont [MPL_W_MENU_PL];
}

void
managePlaylistSave (managepl_t *managepl)
{
  char      *name;
  bool      notvalid = false;
  pltype_t  pltype;

  logProcBegin ();
  if (managepl->ploldname == NULL) {
    logProcEnd ("no-old-name");
    return;
  }

  name = manageGetEntryValue (managepl->wcont [MPL_W_PL_NAME]);

  managepl->changed = managePlaylistCheckChanged (managepl);
  notvalid = false;
  if (uiEntryIsNotValid (managepl->wcont [MPL_W_PL_NAME])) {
    mdfree (name);
    name = mdstrdup (managepl->ploldname);
    uiEntrySetValue (managepl->wcont [MPL_W_PL_NAME], managepl->ploldname);
    notvalid = true;
  }

  /* the playlist has been renamed */
  if (strcmp (managepl->ploldname, name) != 0) {
    playlistRename (managepl->ploldname, name);
    playlistSetName (managepl->playlist, name);
    managepl->changed = true;
  }

  pltype = playlistGetConfigNum (managepl->playlist, PLAYLIST_TYPE);

  if (! managepl->changed) {
    mdfree (name);
    return;
  }

  manageSetPlaylistName (managepl, name);
  managePlaylistUpdatePlaylist (managepl);

  if (! playlistCheck (managepl->playlist)) {
    mdfree (name);
    return;
  }

  playlistSave (managepl->playlist, name);
  /* callback to load the songlist/sequence into the music manager */
  if (managepl->plloadcb != NULL &&
      (pltype == PLTYPE_SONGLIST ||
      pltype == PLTYPE_SEQUENCE ||
      pltype == PLTYPE_PODCAST)) {
    callbackHandlerS (managepl->plloadcb, name);
  }

  mdfree (name);

  if (notvalid) {
    /* set the message after the entry field has been reset */
    /* CONTEXT: Saving Playlist: Error message for invalid playlist name. */
    uiLabelSetText (managepl->minfo->errorMsg, _("Invalid name. Using old name."));
  }

  logProcEnd ("");
}

/* the current playlist may be renamed or deleted. */
/* check for this and if the playlist has */
/* disappeared, reset */
void
managePlaylistLoadCheck (managepl_t *managepl)
{
  char        *name;

  logProcBegin ();
  if (managepl->ploldname == NULL) {
    logProcEnd ("no-old-name");
    return;
  }

  name = manageGetEntryValue (managepl->wcont [MPL_W_PL_NAME]);

  if (! playlistExists (name)) {
    managePlaylistNew (managepl, MANAGE_STD, PLTYPE_AUTO);
  }
  mdfree (name);
  logProcEnd ("");
}

void
managePlaylistLoadFile (managepl_t *managepl, const char *plname,
    int preloadflag)
{
  playlist_t  *pl;

  if (managepl->inload) {
    return;
  }
  logProcBegin ();

  logMsg (LOG_DBG, LOG_ACTIONS, "load playlist file");
  managepl->inload = true;

  if (preloadflag == MANAGE_STD) {
    managePlaylistSave (managepl);
  }

  pl = playlistLoad (plname, NULL, NULL);

  if (pl == NULL) {
    managePlaylistNew (managepl, preloadflag, PLTYPE_AUTO);
    managepl->inload = false;
    logProcEnd ("null");
    return;
  }

  if (managepl->wcont [MPL_W_MENUITEM_DELETE] != NULL) {
    uiWidgetSetState (managepl->wcont [MPL_W_MENUITEM_DELETE], UIWIDGET_ENABLE);
    /* CONTEXT: edit sequences: the name for the special playlist used for the 'queue dance' button */
    if (strcmp (playlistGetName (pl), _("QueueDance")) == 0 ||
        strcmp (playlistGetName (pl), "QueueDance") == 0) {
      uiWidgetSetState (managepl->wcont [MPL_W_MENUITEM_DELETE], UIWIDGET_DISABLE);
    }
  }

  playlistFree (managepl->playlist);
  managepl->playlist = pl;
  manageSetPlaylistName (managepl, plname);
  managePlaylistUpdateData (managepl);

  if (preloadflag == MANAGE_STD && managepl->plloadcb != NULL) {
    pltype_t    pltype;

    pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);
    /* callback to load the songlist/sequence into the music manager */
    if (managepl->plloadcb != NULL &&
        (pltype == PLTYPE_SONGLIST ||
        pltype == PLTYPE_SEQUENCE ||
        pltype == PLTYPE_PODCAST)) {
      callbackHandlerS (managepl->plloadcb, plname);
    }
  }

  manageResetChanged (managepl);
  managepl->inload = false;
  logProcEnd ("");
}

bool
managePlaylistNew (managepl_t *managepl, int preloadflag, int type)
{
  char        tbuff [MAX_PL_NM_LEN];
  playlist_t  *pl = NULL;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new playlist");
  uiLabelSetText (managepl->minfo->statusMsg, "");
  if (preloadflag == MANAGE_STD) {
    managePlaylistSave (managepl);
  }

  /* CONTEXT: playlist management: default name for a new playlist */
  stpecpy (tbuff, tbuff + sizeof (tbuff), _("New Playlist"));
  manageSetPlaylistName (managepl, tbuff);
  managepl->plbackupcreated = false;

  pl = playlistCreate (tbuff, type, NULL, NULL);
  playlistFree (managepl->playlist);
  managepl->playlist = pl;
  manageResetChanged (managepl);
  managePlaylistUpdateData (managepl);

  logProcEnd ("");
  return UICB_CONT;
}

/* internal routines */

static bool
managePlaylistLoad (void *udata)
{
  managepl_t  *managepl = udata;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load playlist");
  uiLabelSetText (managepl->minfo->statusMsg, "");
  managePlaylistSave (managepl);
  selectFileDialog (SELFILE_PLAYLIST, managepl->minfo->window,
      managepl->minfo->options, managepl->callbacks [MPL_CB_SEL_FILE]);
  logProcEnd ("");
  return UICB_CONT;
}

static int32_t
managePlaylistLoadCB (void *udata, const char *plname)
{
  managepl_t  *managepl = udata;

  logProcBegin ();
  managePlaylistLoadFile (managepl, plname, MANAGE_STD);
  logProcEnd ("");
  return 0;
}

static bool
managePlaylistNewAutoCB (void *udata)
{
  managepl_t  *managepl = udata;

  logProcBegin ();
  managePlaylistNew (managepl, MANAGE_STD, PLTYPE_AUTO);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
managePlaylistNewPodcastCB (void *udata)
{
  managepl_t  *managepl = udata;

  logProcBegin ();
  managePlaylistNew (managepl, MANAGE_STD, PLTYPE_PODCAST);
  logProcEnd ("");
  return UICB_CONT;
}

static void
managePlaylistUpdateData (managepl_t *managepl)
{
  pltype_t    pltype;
  playlist_t  *pl;
  char        tbuff [MAX_PL_NM_LEN];

  logProcBegin ();
  pl = managepl->playlist;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  manageplDanceSetPlaylist (managepl->mpldnc, pl);

  if (pltype == PLTYPE_PODCAST) {
    uiWidgetHide (managepl->wcont [MPL_W_STD_VBOX]);
    uiWidgetShow (managepl->wcont [MPL_W_PODCAST_VBOX]);
  } else {
    uiWidgetShow (managepl->wcont [MPL_W_STD_VBOX]);
    uiWidgetHide (managepl->wcont [MPL_W_PODCAST_VBOX]);
  }

  if (pltype == PLTYPE_SONGLIST || pltype == PLTYPE_PODCAST) {
    uiWidgetHide (managepl->wcont [MPL_W_AUTO_SEQ_VBOX]);
  }
  if (pltype == PLTYPE_SEQUENCE || pltype == PLTYPE_AUTO) {
    uiWidgetShow (managepl->wcont [MPL_W_AUTO_SEQ_VBOX]);
  }

  if (pltype == PLTYPE_SONGLIST) {
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Song List"));
  } else if (pltype == PLTYPE_SEQUENCE) {
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Sequence"));
  } else if (pltype == PLTYPE_AUTO) {
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Automatic"));
  } else if (pltype == PLTYPE_PODCAST) {
    /* CONTEXT: playlist management: type of playlist */
    uiLabelSetText (managepl->wcont [MPL_W_PL_TYPE], _("Podcast"));
  }

  uisbnumSetValue (managepl->sbnum [MPL_SB_MAX_PLAY_TIME],
      playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME));
  /* convert the hh:mm value to mm:ss for stop-at */
  uisbnumSetValue (managepl->sbnum [MPL_SB_STOP_AT],
      (double) playlistGetConfigNum (pl, PLAYLIST_STOP_TIME) / 60.0);
  uisbnumSetValue (managepl->sbnum [MPL_SB_STOP_AFTER],
      (double) playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER));
  uisbnumSetValue (managepl->sbnum [MPL_SB_GAP],
      (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0);
  uiSwitchSetValue (managepl->wcont [MPL_W_PLAY_ANN],
      playlistGetConfigNum (pl, PLAYLIST_ANNOUNCE));
  uiratingSetValue (managepl->uirating,
      playlistGetConfigNum (pl, PLAYLIST_RATING));
  uilevelSetValue (managepl->uilowlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH));
  playlistGetConfigListStr (pl, PLAYLIST_ALLOWED_KEYWORDS, tbuff, sizeof (tbuff));
  uiEntrySetValue (managepl->wcont [MPL_W_ALLOWED_KEYWORDS], tbuff);
  playlistGetConfigListStr (pl, PLAYLIST_TAGS, tbuff, sizeof (tbuff));
  uiEntrySetValue (managepl->wcont [MPL_W_TAGS], tbuff);
  uisbnumSetValue (managepl->sbnum [MPL_SB_TAG_WEIGHT],
      (double) playlistGetConfigNum (pl, PLAYLIST_TAG_WEIGHT));

  if (pltype == PLTYPE_PODCAST) {
    uiEntrySetValue (managepl->wcont [MPL_W_URI],
        playlistGetPodcastStr (pl, PODCAST_URI));
    uiEntrySetValue (managepl->wcont [MPL_W_TITLE],
        playlistGetPodcastStr (pl, PODCAST_TITLE));
    uisbnumSetValue (managepl->sbnum [MPL_SB_RETAIN],
        playlistGetPodcastNum (pl, PODCAST_RETAIN));
  }

  managepl->plbackupcreated = false;
  logProcEnd ("");
}

static bool
managePlaylistCopy (void *udata)
{
  managepl_t  *managepl = udata;
  char        *oname;
  char        newname [MAX_PL_NM_LEN];

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy playlist");
  managePlaylistSave (managepl);

  oname = manageGetEntryValue (managepl->wcont [MPL_W_PL_NAME]);
  /* CONTEXT: playlist management: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (managepl->minfo->errorMsg, oname, newname)) {
    manageSetPlaylistName (managepl, newname);
    managepl->plbackupcreated = false;
    manageResetChanged (managepl);
    if (managepl->plloadcb != NULL) {
      callbackHandlerS (managepl->plloadcb, newname);
    }
  }
  mdfree (oname);

  logProcEnd ("");
  return UICB_CONT;
}

static bool
managePlaylistDelete (void *udata)
{
  managepl_t  *managepl = udata;
  char        *oname;

  logProcBegin ();
  logMsg (LOG_DBG, LOG_ACTIONS, "= action: delete playlist");
  oname = manageGetEntryValue (managepl->wcont [MPL_W_PL_NAME]);
  manageDeletePlaylist (managepl->minfo->musicdb, oname);
  manageResetChanged (managepl);
  managePlaylistNew (managepl, MANAGE_STD, PLTYPE_AUTO);
  manageDeleteStatus (managepl->minfo->statusMsg, oname);
  mdfree (oname);
  logProcEnd ("");
  return UICB_CONT;
}

static void
manageSetPlaylistName (managepl_t *managepl, const char *name)
{
  logProcBegin ();
  dataFree (managepl->ploldname);
  managepl->ploldname = mdstrdup (name);
  uiEntrySetValue (managepl->wcont [MPL_W_PL_NAME], name);
  logProcEnd ("");
}

static void
managePlaylistUpdatePlaylist (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  double        dval;
  const char    *tstr;
  pltype_t      pltype;

  logProcBegin ();
  pl = managepl->playlist;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

  manageplDanceSetPlaylist (managepl->mpldnc, pl);

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_MAX_PLAY_TIME]);
  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, tval);

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_STOP_AT]);
  /* convert the mm:ss value to hh:mm for stop-time */
  tval *= 60;
  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, tval);

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_STOP_AFTER]);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, tval);

  dval = uisbnumGetValue (managepl->sbnum [MPL_SB_GAP]);
  playlistSetConfigNum (pl, PLAYLIST_GAP, (long) (dval * 1000.0));

  tval = uiSwitchGetValue (managepl->wcont [MPL_W_PLAY_ANN]);
  playlistSetConfigNum (pl, PLAYLIST_ANNOUNCE, tval);

  tval = uiratingGetValue (managepl->uirating);
  playlistSetConfigNum (pl, PLAYLIST_RATING, tval);

  tval = uilevelGetValue (managepl->uilowlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, tval);

  tval = uilevelGetValue (managepl->uihighlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, tval);

  tstr = uiEntryGetValue (managepl->wcont [MPL_W_ALLOWED_KEYWORDS]);
  playlistSetConfigList (pl, PLAYLIST_ALLOWED_KEYWORDS, tstr);

  tstr = uiEntryGetValue (managepl->wcont [MPL_W_TAGS]);
  playlistSetConfigList (pl, PLAYLIST_TAGS, tstr);

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_TAG_WEIGHT]);
  playlistSetConfigNum (pl, PLAYLIST_TAG_WEIGHT, tval);

  if (pltype == PLTYPE_PODCAST) {
    tstr = uiEntryGetValue (managepl->wcont [MPL_W_URI]);
    playlistSetPodcastStr (pl, PODCAST_URI, tstr);
    tstr = uiEntryGetValue (managepl->wcont [MPL_W_TITLE]);
    playlistSetPodcastStr (pl, PODCAST_TITLE, tstr);
    tval = uisbnumGetValue (managepl->sbnum [MPL_SB_RETAIN]);
    playlistSetPodcastNum (pl, PODCAST_RETAIN, tval);
  }

  logProcEnd ("");
}

static bool
managePlaylistCheckChanged (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  double        dval;
  const char    *svala;
  const char    *svalb;

  logProcBegin ();

  /* check the fields in the user interface */

  if (manageplDanceIsChanged (managepl->mpldnc)) {
    managepl->changed = true;
  }

  if (uisbnumIsChanged (managepl->sbnum [MPL_SB_MAX_PLAY_TIME])) {
    managepl->changed = true;
  }

  if (uisbnumIsChanged (managepl->sbnum [MPL_SB_STOP_AT])) {
    managepl->changed = true;
  }

  if (uisbnumIsChanged (managepl->sbnum [MPL_SB_GAP])) {
    managepl->changed = true;
  }

  /* podcast */

  if (uiEntryChanged (managepl->wcont [MPL_W_URI])) {
    managepl->changed = true;
  }

  if (uiEntryChanged (managepl->wcont [MPL_W_TITLE])) {
    managepl->changed = true;
  }

  if (uisbnumIsChanged (managepl->sbnum [MPL_SB_RETAIN])) {
    managepl->changed = true;
  }

  if (managepl->changed) {
    return managepl->changed;
  }

  /* compare the playlist's old values against the */
  /* values set in the ui */

  pl = managepl->playlist;

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_STOP_AFTER]);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER)) {
    managepl->changed = true;
  }

  dval = uisbnumGetValue (managepl->sbnum [MPL_SB_GAP]);
  if (dval != (double) playlistGetConfigNum (pl, PLAYLIST_GAP) / 1000.0 ) {
    managepl->changed = true;
  }

  tval = uiSwitchGetValue (managepl->wcont [MPL_W_PLAY_ANN]);
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

  tval = uisbnumGetValue (managepl->sbnum [MPL_SB_TAG_WEIGHT]);
  if (tval != playlistGetConfigNum (pl, PLAYLIST_TAG_WEIGHT)) {
    managepl->changed = true;
  }

  /* podcast */

  svala = uiEntryGetValue (managepl->wcont [MPL_W_URI]);
  svalb = playlistGetPodcastStr (pl, PODCAST_URI);
  if ((svalb == NULL && svala != NULL && *svala) ||
      (svala != NULL && svalb != NULL && strcmp (svala, svalb) != 0)) {
    managepl->changed = true;
  }

  svala = uiEntryGetValue (managepl->wcont [MPL_W_TITLE]);
  svalb = playlistGetPodcastStr (pl, PODCAST_TITLE);
  if ((svalb == NULL && svala != NULL && *svala) ||
      (svala != NULL && svalb != NULL && strcmp (svala, svalb) != 0)) {
    managepl->changed = true;
  }

  dval = uisbnumGetValue (managepl->sbnum [MPL_SB_RETAIN]);
  if (dval != (double) playlistGetPodcastNum (pl, PODCAST_RETAIN)) {
    managepl->changed = true;
  }

  logProcEnd ("");
  return managepl->changed;
}

static int
managePlaylistTextEntryChg (uiwcont_t *e, const char *label, void *udata)
{
  managepl_t *managepl = udata;

  managepl->changed = true;
  return UIENTRY_OK;
}

static void
manageResetChanged (managepl_t *managepl)
{
  uisbnumClearChanged (managepl->sbnum [MPL_SB_MAX_PLAY_TIME]);
  uisbnumClearChanged (managepl->sbnum [MPL_SB_STOP_AT]);
  uisbnumClearChanged (managepl->sbnum [MPL_SB_GAP]);
  managepl->changed = false;
}

static bool
managePlaylistValMaxPlayTimeCallback (void *udata)
{
  return managePlaylistValidateCallback (udata, MPL_SB_MAX_PLAY_TIME);
}

static bool
managePlaylistValStopAtCallback (void *udata)
{
  return managePlaylistValidateCallback (udata, MPL_SB_STOP_AT);
}

static bool
managePlaylistValidateCallback (managepl_t *managepl, int idx)
{
  uiLabelSetText (managepl->minfo->errorMsg, "");
  if (! uisbnumIsValid (managepl->sbnum [idx])) {
    const char  *tmsg;

    tmsg = uisbnumGetValidationError (managepl->sbnum [idx]);
    uiLabelSetText (managepl->minfo->errorMsg, tmsg);
    return UICB_STOP;
  }

  return UICB_CONT;
}

