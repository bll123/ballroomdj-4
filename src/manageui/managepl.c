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
#include "manageui.h"
#include "pathbld.h"
#include "playlist.h"
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
  MPL_CB_MAXPLAYTIME,
  MPL_CB_STOPAT,
  MPL_CB_MAX,
};

typedef struct managepl {
  UIWidget        *windowp;
  nlist_t         *options;
  UIWidget        *statusMsg;
  uimenu_t        plmenu;
  UICallback      callbacks [MPL_CB_MAX];
  char            *ploldname;
  bool            plbackupcreated;
  uientry_t       plname;
  pltype_t        pltype;
  uispinbox_t     uimaxplaytime;
  uispinbox_t     uistopat;
  UIWidget        uistopafter;
  UIWidget        uigap;
  uirating_t      *uirating;
  UIWidget        uiratingitem;
  uilevel_t       *uilowlevel;
  UIWidget        uilowlevelitem;
  uilevel_t       *uihighlevel;
  UIWidget        uihighlevelitem;
  uientry_t       allowedkeywords;
  UIWidget        uiallowedkeywordsitem;
  UIWidget        uipltype;
  managepltree_t  *managepltree;
  playlist_t      *playlist;
} managepl_t;

static bool managePlaylistLoad (void *udata);
static bool managePlaylistCopy (void *udata);
static void managePlaylistLoadFile (void *udata, const char *fn);
static void managePlaylistUpdateData (managepl_t *managepl);
static bool managePlaylistNew (void *udata);
static void manageSetPlaylistName (managepl_t *managepl, const char *nm);
static long managePlaylistValMSCallback (void *udata, const char *txt);
static long managePlaylistValHMCallback (void *udata, const char *txt);
static void managePlaylistUpdatePlaylist (managepl_t *managepl);

managepl_t *
managePlaylistAlloc (UIWidget *window, nlist_t *options, UIWidget *statusMsg)
{
  managepl_t *managepl;

  managepl = malloc (sizeof (managepl_t));
  uiutilsUIWidgetInit (&managepl->uipltype);
  managepl->ploldname = NULL;
  managepl->plbackupcreated = false;
  uiMenuInit (&managepl->plmenu);
  uiEntryInit (&managepl->plname, 20, 50);
  managepl->statusMsg = statusMsg;
  managepl->windowp = window;
  managepl->options = options;
  managepl->pltype = PLTYPE_AUTO;
  uiutilsUIWidgetInit (&managepl->uistopafter);
  uiutilsUIWidgetInit (&managepl->uigap);
  managepl->managepltree = NULL;
  managepl->uirating = NULL;
  managepl->uilowlevel = NULL;
  managepl->uihighlevel = NULL;
  uiutilsUIWidgetInit (&managepl->uiratingitem);
  uiutilsUIWidgetInit (&managepl->uilowlevelitem);
  uiutilsUIWidgetInit (&managepl->uihighlevelitem);
  uiEntryInit (&managepl->allowedkeywords, 15, 50);
  uiutilsUIWidgetInit (&managepl->uiallowedkeywordsitem);
  managepl->playlist = NULL;

  return managepl;
}

void
managePlaylistFree (managepl_t *managepl)
{
  if (managepl != NULL) {
    if (managepl->ploldname != NULL) {
      free (managepl->ploldname);
    }
    if (managepl->managepltree != NULL) {
      managePlaylistTreeFree (managepl->managepltree);
    }
    uiEntryFree (&managepl->plname);
    uiEntryFree (&managepl->allowedkeywords);
    uiratingFree (managepl->uirating);
    uilevelFree (managepl->uilowlevel);
    uilevelFree (managepl->uihighlevel);
    if (managepl->playlist != NULL) {
      playlistFree (managepl->playlist);
    }
    free (managepl);
  }
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

  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);
  uiCreateSizeGroupHoriz (&sg);
  uiCreateSizeGroupHoriz (&sgA);
  uiCreateSizeGroupHoriz (&sgB);
  uiCreateSizeGroupHoriz (&sgC);

  uiWidgetSetAllMargins (vboxp, uiBaseMarginSz * 2);

  uiCreateHorizBox (&tophbox);
  uiWidgetAlignVertStart (&tophbox);
  uiBoxPackStart (vboxp, &tophbox);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&tophbox, &hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiCreateColonLabel (&uiwidget, _("Playlist"));
  uiBoxPackStart (&hbox, &uiwidget);

  uiEntryCreate (&managepl->plname);
  uiEntrySetColor (&managepl->plname, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  /* CONTEXT: playlist management: default name for a new playlist */
  uiBoxPackStart (&hbox, &managepl->plname.uientry);

  uiCreateHorizBox (&hbox);
  uiWidgetSetMarginStart (&hbox, uiBaseMarginSz * 20);
  uiBoxPackStart (&tophbox, &hbox);

  /* CONTEXT: playlist management: label for playlist name */
  uiCreateColonLabel (&uiwidget, _("Playlist Type"));
  uiBoxPackStart (&hbox, &uiwidget);

  /* CONTEXT: playlist management: default playlist type */
  uiCreateLabel (&uiwidget, _("Automatic"));
  uiLabelSetColor (&uiwidget, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
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

  uiutilsUICallbackStrInit (&managepl->callbacks [MPL_CB_MAXPLAYTIME],
      managePlaylistValMSCallback, managepl);
  uiSpinboxTimeCreate (&managepl->uimaxplaytime, managepl, &managepl->callbacks [MPL_CB_MAXPLAYTIME]);
  uiwidgetp = uiSpinboxGetUIWidget (&managepl->uimaxplaytime);
  uiBoxPackStart (&hbox, uiwidgetp);
  uiSizeGroupAdd (&sgA, uiwidgetp);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: stop at */
  uiCreateColonLabel (&uiwidget, _("Stop At"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiutilsUICallbackStrInit (&managepl->callbacks [MPL_CB_STOPAT],
      managePlaylistValHMCallback, managepl);
  uiSpinboxTimeCreate (&managepl->uistopat, managepl,
      &managepl->callbacks [MPL_CB_STOPAT]);
  uiSpinboxSetRange (&managepl->uistopat, 0, 1440000);
  uiSpinboxWrap (&managepl->uistopat);
  uiwidgetp = uiSpinboxGetUIWidget (&managepl->uistopat);
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

  /* CONTEXT: playlist management: Gap */
  uiCreateColonLabel (&uiwidget, _("Gap"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiSpinboxIntCreate (&uiwidget);
  uiSpinboxSet (&uiwidget, 0.0, 60.0);
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sgB, &uiwidget);
  uiutilsUIWidgetCopy (&managepl->uigap, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);

  /* CONTEXT: playlist management: Play Announcements */
  uiCreateColonLabel (&uiwidget, _("Play Announcements"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiCreateSwitch (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  /* automatic and sequenced playlists; keep the widget so these */
  /* can be hidden */

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uiratingitem, &hbox);

  /* CONTEXT: playlist management: Dance Rating */
  uiCreateColonLabel (&uiwidget, _("Dance Rating"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uirating = uiratingSpinboxCreate (&hbox, false);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uilowlevelitem, &hbox);

  /* CONTEXT: playlist management: Low Dance Level */
  uiCreateColonLabel (&uiwidget, _("Low Dance Level"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uilowlevel = uilevelSpinboxCreate (&hbox, false);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uihighlevelitem, &hbox);

  /* CONTEXT: playlist management: High Dance Level */
  uiCreateColonLabel (&uiwidget, _("High Dance Level"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  managepl->uihighlevel = uilevelSpinboxCreate (&hbox, false);

  uiCreateHorizBox (&hbox);
  uiBoxPackStart (&lcol, &hbox);
  uiutilsUIWidgetCopy (&managepl->uiallowedkeywordsitem, &hbox);

  /* CONTEXT: playlist management: allowed keywords */
  uiCreateColonLabel (&uiwidget, _("Allowed Keywords"));
  uiBoxPackStart (&hbox, &uiwidget);
  uiSizeGroupAdd (&sg, &uiwidget);

  uiEntryCreate (&managepl->allowedkeywords);
  uiBoxPackStart (&hbox, &managepl->allowedkeywords.uientry);

  /* right side to hold the tree */
  uiCreateVertBox (&rcol);
  uiWidgetSetMarginStart (&rcol, uiBaseMarginSz * 8);
  uiBoxPackStartExpand (&mainhbox, &rcol);

  managepl->managepltree = managePlaylistTreeAlloc (managepl->statusMsg);
  manageBuildUIPlaylistTree (managepl->managepltree, &rcol, &tophbox);
  managePlaylistNew (managepl);
}

uimenu_t *
managePlaylistMenu (managepl_t *managepl, UIWidget *uimenubar)
{
  UIWidget  menu;
  UIWidget  menuitem;

  if (! managepl->plmenu.initialized) {
    uiMenuAddMainItem (uimenubar, &menuitem,
        /* CONTEXT: menu selection: sequence: edit menu */
        &managepl->plmenu, _("Edit"));

    uiCreateSubMenu (&menuitem, &menu);

    /* CONTEXT: menu selection: playlist: edit menu: load */
    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_LOAD],
        managePlaylistLoad, managepl);
    uiMenuCreateItem (&menu, &menuitem, _("Load"),
        &managepl->callbacks [MPL_CB_MENU_PL_LOAD]);

    /* CONTEXT: menu selection: playlist: edit menu: create copy */
    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_COPY],
        managePlaylistCopy, managepl);
    uiMenuCreateItem (&menu, &menuitem, _("Create Copy"),
        &managepl->callbacks [MPL_CB_MENU_PL_COPY]);

    /* CONTEXT: menu selection: playlist: edit menu: new automatic sequence */
    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_NEW],
        managePlaylistNew, managepl);
    uiMenuCreateItem (&menu, &menuitem, _("New Automatic Playlist"),
        &managepl->callbacks [MPL_CB_MENU_PL_NEW]);

    managepl->plmenu.initialized = true;
  }

  uiMenuDisplay (&managepl->plmenu);
  return &managepl->plmenu;
}

void
managePlaylistSave (managepl_t *managepl)
{
  const char  *name;

  if (managepl->ploldname == NULL) {
    return;
  }

  name = uiEntryGetValue (&managepl->plname);

  /* the playlist has been renamed */
  if (strcmp (managepl->ploldname, name) != 0) {
    manageRenamePlaylistFiles (managepl->ploldname, name);
  }

  managePlaylistUpdatePlaylist (managepl);
  playlistSave (managepl->playlist);
}

/* internal routines */

static bool
managePlaylistLoad (void *udata)
{
  managepl_t  *managepl = udata;

  selectFileDialog (SELFILE_PLAYLIST, managepl->windowp, managepl->options,
      managepl, managePlaylistLoadFile);
  return UICB_CONT;
}

static void
managePlaylistLoadFile (void *udata, const char *fn)
{
  managepl_t  *managepl = udata;
  playlist_t  *pl;

  managePlaylistSave (managepl);

  pl = playlistAlloc (NULL);
  playlistLoad (pl, fn);
  if (managepl->playlist != NULL) {
    playlistFree (managepl->playlist);
  }
  managepl->playlist = pl;
  manageSetPlaylistName (managepl, fn);

  managePlaylistUpdateData (managepl);
}

static void
managePlaylistUpdateData (managepl_t *managepl)
{
  pltype_t    pltype;
  playlist_t  *pl;

  pl = managepl->playlist;
  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);

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

  uiSpinboxTimeSetValue (&managepl->uimaxplaytime,
      playlistGetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME));
  uiSpinboxTimeSetValue (&managepl->uistopat,
      playlistGetConfigNum (pl, PLAYLIST_STOP_TIME));
  uiSpinboxSetValue (&managepl->uistopafter,
      playlistGetConfigNum (pl, PLAYLIST_STOP_AFTER));
  uiSpinboxSetValue (&managepl->uigap,
      playlistGetConfigNum (pl, PLAYLIST_GAP));
  uiratingSetValue (managepl->uirating,
      playlistGetConfigNum (pl, PLAYLIST_RATING));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_LOW));
  uilevelSetValue (managepl->uihighlevel,
      playlistGetConfigNum (pl, PLAYLIST_LEVEL_HIGH));

  managepl->plbackupcreated = false;
}

static bool
managePlaylistCopy (void *udata)
{
  managepl_t *managepl = udata;
  const char  *oname;
  char        newname [200];

  managePlaylistSave (managepl);

  oname = uiEntryGetValue (&managepl->plname);
  /* CONTEXT: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (managepl->statusMsg, oname, newname)) {
    manageSetPlaylistName (managepl, newname);
    managepl->plbackupcreated = false;
  }

  return UICB_CONT;
}

static bool
managePlaylistNew (void *udata)
{
  managepl_t  *managepl = udata;
  char        tbuff [200];
  playlist_t  *pl = NULL;

  managePlaylistSave (managepl);

  /* CONTEXT: sequence: default name for a new sequence */
  snprintf (tbuff, sizeof (tbuff), _("New Playlist"));
  manageSetPlaylistName (managepl, tbuff);
  managepl->plbackupcreated = false;

  pl = playlistAlloc (NULL);
  playlistCreate (pl, tbuff, PLTYPE_AUTO, NULL);
  if (managepl->playlist != NULL) {
    playlistFree (managepl->playlist);
  }
  managepl->playlist = pl;
  managePlaylistUpdateData (managepl);

  return UICB_CONT;
}

static void
manageSetPlaylistName (managepl_t *managepl, const char *name)
{
  uiEntrySetValue (&managepl->plname, name);
  if (managepl->ploldname != NULL) {
    free (managepl->ploldname);
  }
  managepl->ploldname = strdup (name);
}

static long
managePlaylistValMSCallback (void *udata, const char *txt)
{
  managepl_t  *managepl = udata;
  const char  *valstr;
  char        tbuff [200];
  long        value;

  uiLabelSetText (managepl->statusMsg, "");
  valstr = validate (txt, VAL_MIN_SEC);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->statusMsg, tbuff);
    return -1;
  }

  value = tmutilStrToMS (txt);
  return value;
}

static long
managePlaylistValHMCallback (void *udata, const char *txt)
{
  managepl_t  *managepl = udata;
  const char  *valstr;
  char        tbuff [200];
  long        value;

  uiLabelSetText (managepl->statusMsg, "");
  valstr = validate (txt, VAL_HOUR_MIN);
  if (valstr != NULL) {
    snprintf (tbuff, sizeof (tbuff), valstr, txt);
    uiLabelSetText (managepl->statusMsg, tbuff);
    return -1;
  }

  value = tmutilStrToHM (txt);
  return value;
}

static void
managePlaylistUpdatePlaylist (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  const char    *tstr;

  pl = managepl->playlist;

  managePlaylistTreeUpdatePlaylist (managepl->managepltree);

  tval = uiSpinboxGetValue (uiSpinboxGetUIWidget (&managepl->uimaxplaytime));
  playlistSetConfigNum (pl, PLAYLIST_MAX_PLAY_TIME, tval);

  tval = uiSpinboxGetValue (uiSpinboxGetUIWidget (&managepl->uistopat));
  playlistSetConfigNum (pl, PLAYLIST_STOP_TIME, tval);

  tval = uiSpinboxGetValue (&managepl->uistopafter);
  playlistSetConfigNum (pl, PLAYLIST_STOP_AFTER, tval);

  tval = uiSpinboxGetValue (&managepl->uigap);
  playlistSetConfigNum (pl, PLAYLIST_GAP, tval);

  tval = uiratingGetValue (managepl->uirating);
  playlistSetConfigNum (pl, PLAYLIST_RATING, tval);

  tval = uilevelGetValue (managepl->uilowlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_LOW, tval);

  tval = uilevelGetValue (managepl->uihighlevel);
  playlistSetConfigNum (pl, PLAYLIST_LEVEL_HIGH, tval);

  tstr = uiEntryGetValue (&managepl->allowedkeywords);
  playlistSetConfigList (pl, PLAYLIST_ALLOWED_KEYWORDS, tstr);
}
