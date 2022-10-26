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
#include "inline.h"
#include "log.h"
#include "manageui.h"
#include "pathbld.h"
#include "playlist.h"
#include "tmutil.h"
#include "tagdef.h"
#include "ui.h"
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
  MPL_CB_PL_LOAD,
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
static bool managePlaylistCopy (void *udata);
static void managePlaylistUpdateData (managepl_t *managepl);
static bool managePlaylistNew (void *udata);
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

  managepl = malloc (sizeof (managepl_t));
  uiutilsUIWidgetInit (&managepl->uipltype);
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
    if (managepl->playlist != NULL) {
      playlistFree (managepl->playlist);
    }
    free (managepl);
  }
}

void
managePlaylistSetLoadCallback (managepl_t *managepl, UICallback *uicb)
{
  if (managepl == NULL) {
    return;
  }
  memcpy (&managepl->callbacks [MPL_CB_PL_LOAD], uicb, sizeof (UICallback));
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
  uiCreateSizeGroupHoriz (&sg);   // labels
  uiCreateSizeGroupHoriz (&sgA);  // time widgets + gap widget
  uiCreateSizeGroupHoriz (&sgB);  // numeric widgets
  uiCreateSizeGroupHoriz (&sgC);  // text widgets

  uiWidgetSetAllMargins (vboxp, uiBaseMarginSz * 2);

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
  uiEntrySetColor (managepl->plname, bdjoptGetStr (OPT_P_UI_ACCENT_COL));
  uiBoxPackStart (&hbox, uiEntryGetUIWidget (managepl->plname));

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
  uiSpinboxTimeCreate (managepl->uimaxplaytime, managepl, &managepl->callbacks [MPL_CB_MAXPLAYTIME]);
  uiwidgetp = uiSpinboxGetUIWidget (managepl->uimaxplaytime);
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
  uiSpinboxTimeCreate (managepl->uistopat, managepl,
      &managepl->callbacks [MPL_CB_STOPAT]);
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

  /* CONTEXT: playlist management: Gap */
  uiCreateColonLabel (&uiwidget, _("Gap"));
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
  uiWidgetSetMarginStart (&rcol, uiBaseMarginSz * 8);
  uiBoxPackStartExpand (&mainhbox, &rcol);

  managepl->managepltree = managePlaylistTreeAlloc (managepl->statusMsg);
  manageBuildUIPlaylistTree (managepl->managepltree, &rcol, &tophbox);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managePlaylistNew (managepl);
  managepl->changed = false;
}

uimenu_t *
managePlaylistMenu (managepl_t *managepl, UIWidget *uimenubar)
{
  UIWidget  menu;
  UIWidget  menuitem;

  if (! managepl->plmenu.initialized) {
    uiMenuAddMainItem (uimenubar, &menuitem,
        /* CONTEXT: playlist management: menu selection: playlist: edit menu */
        &managepl->plmenu, _("Edit"));

    uiCreateSubMenu (&menuitem, &menu);

    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_LOAD],
        managePlaylistLoad, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: load */
    uiMenuCreateItem (&menu, &menuitem, _("Load"),
        &managepl->callbacks [MPL_CB_MENU_PL_LOAD]);

    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_NEW],
        managePlaylistNew, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: new automatic playlist */
    uiMenuCreateItem (&menu, &menuitem, _("New Automatic Playlist"),
        &managepl->callbacks [MPL_CB_MENU_PL_NEW]);

    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_COPY],
        managePlaylistCopy, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: create copy */
    uiMenuCreateItem (&menu, &menuitem, _("Create Copy"),
        &managepl->callbacks [MPL_CB_MENU_PL_COPY]);

    uiutilsUICallbackInit (&managepl->callbacks [MPL_CB_MENU_PL_DELETE],
        managePlaylistDelete, managepl, NULL);
    /* CONTEXT: playlist management: menu selection: playlist: edit menu: delete playlist */
    uiMenuCreateItem (&menu, &menuitem, _("Delete"),
        &managepl->callbacks [MPL_CB_MENU_PL_DELETE]);

    managepl->plmenu.initialized = true;
  }

  uiMenuDisplay (&managepl->plmenu);
  return &managepl->plmenu;
}

void
managePlaylistSave (managepl_t *managepl)
{
  char  *name;

  if (managepl->ploldname == NULL) {
    return;
  }

  name = strdup (uiEntryGetValue (managepl->plname));

  /* the playlist has been renamed */
  if (strcmp (managepl->ploldname, name) != 0) {
    manageRenamePlaylistFiles (managepl->ploldname, name);
    managepl->changed = true;
  }
  managepl->changed = managePlaylistCheckChanged (managepl);

  if (managepl->changed) {
    manageSetPlaylistName (managepl, name);
    managePlaylistUpdatePlaylist (managepl);
    playlistSave (managepl->playlist, name);
  }
  free (name);
}

/* the current playlist may be renamed or deleted. */
/* check for this and if the playlist has */
/* disappeared, reset */
void
managePlaylistLoadCheck (managepl_t *managepl)
{
  const char  *name;

  if (managepl->ploldname == NULL) {
    return;
  }

  name = uiEntryGetValue (managepl->plname);

  if (! managePlaylistExists (name)) {
    managePlaylistNew (managepl);
  }
}

void
managePlaylistLoadFile (void *udata, const char *fn)
{
  managepl_t  *managepl = udata;
  playlist_t  *pl;
  pltype_t    pltype;

  if (managepl->inload) {
    return;
  }

  logMsg (LOG_DBG, LOG_ACTIONS, "load playlist file");
  managepl->inload = true;

  managePlaylistSave (managepl);

  pl = playlistAlloc (NULL);
  playlistLoad (pl, fn);
  if (managepl->playlist != NULL) {
    playlistFree (managepl->playlist);
  }
  managepl->playlist = pl;
  manageSetPlaylistName (managepl, fn);
  managePlaylistUpdateData (managepl);

  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);

  pltype = playlistGetConfigNum (pl, PLAYLIST_TYPE);
  if (pltype == PLTYPE_SONGLIST) {
    uiutilsCallbackStrHandler (&managepl->callbacks [MPL_CB_PL_LOAD], fn);
  }

  managepl->changed = false;
  managepl->inload = false;
}

/* internal routines */

static bool
managePlaylistLoad (void *udata)
{
  managepl_t  *managepl = udata;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: load playlist");
  selectFileDialog (SELFILE_PLAYLIST, managepl->windowp, managepl->options,
      managepl, managePlaylistLoadFile);
  return UICB_CONT;
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
}

static bool
managePlaylistCopy (void *udata)
{
  managepl_t *managepl = udata;
  const char  *oname;
  char        newname [200];

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: copy playlist");
  managePlaylistSave (managepl);

  oname = uiEntryGetValue (managepl->plname);
  /* CONTEXT: playlist management: the new name after 'create copy' (e.g. "Copy of DJ-2022-04") */
  snprintf (newname, sizeof (newname), _("Copy of %s"), oname);
  if (manageCreatePlaylistCopy (managepl->statusMsg, oname, newname)) {
    manageSetPlaylistName (managepl, newname);
    managepl->plbackupcreated = false;
    uiSpinboxResetChanged (managepl->uimaxplaytime);
    uiSpinboxResetChanged (managepl->uistopat);
    uiSpinboxResetChanged (managepl->uigap);
    managepl->changed = false;
  }

  return UICB_CONT;
}

static bool
managePlaylistNew (void *udata)
{
  managepl_t  *managepl = udata;
  char        tbuff [200];
  playlist_t  *pl = NULL;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: new playlist");
  managePlaylistSave (managepl);

  /* CONTEXT: playlist management: default name for a new playlist */
  snprintf (tbuff, sizeof (tbuff), _("New Playlist"));
  manageSetPlaylistName (managepl, tbuff);
  managepl->plbackupcreated = false;

  pl = playlistAlloc (NULL);
  playlistCreate (pl, tbuff, PLTYPE_AUTO);
  if (managepl->playlist != NULL) {
    playlistFree (managepl->playlist);
  }
  managepl->playlist = pl;
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;
  managePlaylistUpdateData (managepl);

  return UICB_CONT;
}

static bool
managePlaylistDelete (void *udata)
{
  managepl_t  *managepl = udata;
  const char  *oname;

  logMsg (LOG_DBG, LOG_ACTIONS, "= action: delete playlist");
  oname = uiEntryGetValue (managepl->plname);
  manageDeletePlaylist (managepl->statusMsg, oname);
  uiSpinboxResetChanged (managepl->uimaxplaytime);
  uiSpinboxResetChanged (managepl->uistopat);
  uiSpinboxResetChanged (managepl->uigap);
  managepl->changed = false;

  managePlaylistNew (managepl);
  managePlaylistUpdateData (managepl);

  return UICB_CONT;
}

static void
manageSetPlaylistName (managepl_t *managepl, const char *name)
{
  dataFree (managepl->ploldname);
  managepl->ploldname = strdup (name);
  uiEntrySetValue (managepl->plname, name);
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
  double        dval;
  const char    *tstr;

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
}

static bool
managePlaylistCheckChanged (managepl_t *managepl)
{
  playlist_t    *pl;
  long          tval;
  double        dval;

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
  if (dval != (double) playlistGetConfigNum (pl, PLAYLIST_GAP) * 1000.0 ) {
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

  return managepl->changed;
}

static int
managePlaylistAllowedKeywordsChg (uientry_t *e, void *udata)
{
  managepl_t *managepl = udata;

  managepl->changed = true;
  return UIENTRY_OK;
}

