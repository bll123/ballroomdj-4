/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "audiosrc.h"   // for file prefix
#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "configui.h"
#include "dirlist.h"
#include "dispsel.h"
#include "dyintfc.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "ilist.h"
#include "mdebug.h"
#include "nlist.h"
#include "orgutil.h"
#include "pathbld.h"
#include "pathdisp.h"
#include "pathinfo.h"
#include "slist.h"
#include "sysvars.h"
#include "ui.h"
#include "webclient.h"

static nlist_t * confuiGetThemeList (void);
static slist_t * confuiGetThemeNames (slist_t *themelist, slist_t *filelist);
static char * confuiMakeQRCodeFile (char *title, char *uri);
static void confuiUpdateOrgExample (org_t *org, const char *data, uiwcont_t *uiwidgetp);
static bool confuiSearchDispSel (confuigui_t *gui, int selidx, const char *disp);
static void confuiCreateTagListingMultDisp (confuigui_t *gui, slist_t *dlist, int sidx, int eidx);

static const char *orgexamples [] = {
  "FILE\n..none.mp3\nDISC\n..1\nTRACKNUMBER\n..1\n"
      "ALBUM\n..Smooth\nALBUMARTIST\n..Santana\n"
      "ARTIST\n..Santana\nDANCE\n..Cha Cha\nTITLE\n..Smooth\n"
      "GENRE\n..Ballroom Dance\n",
  "FILE\n..none2.mp3\nDISC\n..1\nTRACKNUMBER\n..2\n"
      "ALBUM\n..The Ultimate Latin Album 4: Latin Eyes\nALBUMARTIST\n..WRD\n"
      "ARTIST\n..Gizelle D'Cole\nDANCE\n..Rumba\nTITLE\n..Asi\n"
      "GENRE\n..Ballroom Dance\n",
  "FILE\n..none.mp3\nDISC\n..1\nTRACKNUMBER\n..3\n"
      "ALBUM\n..Ballroom Stars 6\nALBUMARTIST\n..Various Artists\n"
      "ARTIST\n..Léa\nDANCE\n..Waltz\nTITLE\n..Je Vole! (from 'La Famille Bélier')\n"
      "GENRE\n..Ballroom Dance",
  /* empty album artist */
  "FILE\n..none4.mp3\nDISC\n..2\nTRACKNUMBER\n..4\n"
      "ALBUM\n..The Ultimate Latin Album 9: Footloose\nALBUMARTIST\n..\n"
      "ARTIST\n..Gloria Estefan\nDANCE\n..Rumba\nTITLE\n..Me voy\n"
      "GENRE\n..Ballroom Dance",
};


/* the theme list is used by both the ui and the marquee selections */
void
confuiLoadThemeList (confuigui_t *gui)
{
  nlist_t     *tlist;
  nlistidx_t  iteridx;
  int         count;
  bool        usesys = false;
  const char  *p;
  const char  *mqtheme;
  const char  *uitheme;

  p = bdjoptGetStr (OPT_MP_UI_THEME);
  /* use the system default if the ui theme is empty */
  if (p == NULL || ! *p) {
    usesys = true;
  }

  mqtheme = bdjoptGetStr (OPT_MP_MQ_THEME);
  uitheme = bdjoptGetStr (OPT_MP_UI_THEME);

  tlist = confuiGetThemeList ();
  nlistStartIterator (tlist, &iteridx);
  count = 0;

  /* need some sort of default */
  gui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx = 0;

  while ((p = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    if (mqtheme != NULL && strcmp (p, mqtheme) == 0) {
      gui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx = count;
    }
    if (! usesys && strcmp (p, uitheme) == 0) {
      gui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx = count;
    }
    if (usesys &&
        strcmp (p, sysvarsGetStr (SV_THEME_DEFAULT)) == 0) {
      gui->uiitem [CONFUI_SPINBOX_UI_THEME].listidx = count;
    }
    ++count;
  }
  /* the theme list is ordered */
  gui->uiitem [CONFUI_SPINBOX_UI_THEME].displist = tlist;
  gui->uiitem [CONFUI_SPINBOX_MQ_THEME].displist = tlist;
}

void
confuiUpdateMobmqQrcode (confuigui_t *gui)
{
  char          uridisp [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  int           type;
  uiwcont_t     *uiwidgetp = NULL;

  logProcBegin ();

  type = bdjoptGetNum (OPT_P_MOBMQ_TYPE);

  confuiSetStatusMsg (gui, "");
  if (type == MOBMQ_TYPE_OFF) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (type == MOBMQ_TYPE_LOCAL) {
    const char  *host;

    host = sysvarsGetStr (SV_HOSTNAME);
    snprintf (uridisp, sizeof (uridisp), "http://%s.local:%" PRId64, host,
        bdjoptGetNum (OPT_P_MOBMQ_PORT));
  }
  if (type == MOBMQ_TYPE_INTERNET) {
    snprintf (uridisp, sizeof (uridisp), "%s/%s?mobmqtag=%s",
        sysvarsGetStr (SV_HOST_MOBMQ), sysvarsGetStr (SV_URI_MOBMQ_HTML),
        bdjoptGetStr (OPT_P_MOBMQ_TAG));
  }

  if (type != MOBMQ_TYPE_OFF) {
    /* CONTEXT: configuration: qr code: title display for mobile marquee */
    qruri = confuiMakeQRCodeFile (_("Mobile Marquee"), uridisp);
  }

  uiwidgetp = gui->uiitem [CONFUI_WIDGET_MOBMQ_QR_CODE].uiwidgetp;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  dataFree (gui->uiitem [CONFUI_WIDGET_MOBMQ_QR_CODE].uri);
  gui->uiitem [CONFUI_WIDGET_MOBMQ_QR_CODE].uri = mdstrdup (qruri);
  if (*qruri) {
    mdfree (qruri);
  }
  logProcEnd ("");
}

void
confuiUpdateRemctrlQrcode (confuigui_t *gui)
{
  char          uridisp [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  bool          enabled;
  uiwcont_t    *uiwidgetp;

  logProcBegin ();

  enabled = bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (! enabled) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (enabled) {
    const char  *host;

    host = sysvarsGetStr (SV_HOSTNAME);
    snprintf (uridisp, sizeof (uridisp), "http://%s.local:%" PRId64, host,
        bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (enabled) {
    /* CONTEXT: configuration: qr code: title display for mobile remote control */
    qruri = confuiMakeQRCodeFile (_("Mobile Remote Control"), uridisp);
  }

  uiwidgetp = gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uiwidgetp;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  dataFree (gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri);
  gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri = mdstrdup (qruri);
  if (*qruri) {
    mdfree (qruri);
  }
  logProcEnd ("");
}

void
confuiUpdateOrgExamples (confuigui_t *gui, const char *orgpath)
{
  org_t     *org;
  uiwcont_t *uiwidgetp;
  int       max;


  if (orgpath == NULL) {
    return;
  }

  logProcBegin ();
  org = orgAlloc (orgpath);

  /* the only genres that are shipped are : */
  /* ballroom dance, jazz, rock, classical */

  max = CONFUI_WIDGET_AO_EXAMPLE_MAX - CONFUI_WIDGET_AO_EXAMPLE_1;
  for (int i = 0; i < max; ++i) {
    uiwidgetp = gui->uiitem [i + CONFUI_WIDGET_AO_EXAMPLE_1].uiwidgetp;
    confuiUpdateOrgExample (org, orgexamples [i], uiwidgetp);
  }

  orgFree (org);
  logProcEnd ("");
}

void
confuiSetStatusMsg (confuigui_t *gui, const char *msg)
{
  uiLabelSetText (gui->statusMsg, msg);
}

void
confuiSelectDirDialog (uisfcb_t *sfcb, const char *startpath,
    const char *title)
{
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin ();
  selectdata = uiSelectInit (sfcb->window, title, startpath, NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (sfcb->entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  logProcEnd ("");
}

void
confuiSelectFileDialog (uisfcb_t *sfcb, const char *startpath,
    const char *mimefiltername, const char *mimetype)
{
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin ();
  selectdata = uiSelectInit (sfcb->window,
      /* CONTEXT: configuration: file selection dialog: window title */
      _("Select File"), startpath, NULL, mimefiltername, mimetype);
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (sfcb->entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    mdfree (fn);
  }
  uiSelectFree (selectdata);
  logProcEnd ("");
}

void
confuiCreateTagListingDisp (confuigui_t *gui)
{
  dispselsel_t  selidx;

  logProcBegin ();

  selidx = uiSpinboxTextGetValue (gui->uiitem [CONFUI_SPINBOX_DISP_SEL].uiwidgetp);

  if (selidx == DISP_SEL_SONGEDIT_A ||
      selidx == DISP_SEL_SONGEDIT_B ||
      selidx == DISP_SEL_SONGEDIT_C) {
    confuiCreateTagListingMultDisp (gui, gui->edittaglist,
        DISP_SEL_SONGEDIT_A, DISP_SEL_SONGEDIT_C);
  } else if (selidx == DISP_SEL_AUDIOID_LIST ||
      selidx == DISP_SEL_AUDIOID) {
    uiduallistSet (gui->dispselduallist, gui->audioidtaglist,
        DL_LIST_SOURCE);
  } else if (selidx == DISP_SEL_MARQUEE) {
    uiduallistSet (gui->dispselduallist, gui->marqueetaglist,
        DL_LIST_SOURCE);
  } else if (selidx == DISP_SEL_CURRSONG) {
    uiduallistSet (gui->dispselduallist, gui->pluitaglist,
        DL_LIST_SOURCE);
  } else {
    uiduallistSet (gui->dispselduallist, gui->listingtaglist,
        DL_LIST_SOURCE);
  }
  logProcEnd ("");
}

void
confuiCreateTagSelectedDisp (confuigui_t *gui)
{
  dispselsel_t  selidx;
  slist_t       *sellist;
  dispsel_t     *dispsel;

  logProcBegin ();

  selidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].uiwidgetp);

  dispsel = gui->dispsel;
  sellist = dispselGetList (dispsel, selidx);

  uiduallistSet (gui->dispselduallist, sellist, DL_LIST_TARGET);
  logProcEnd ("");
}

void
confuiLoadIntfcList (confuigui_t *gui, ilist_t *interfaces,
    int optidx, int optnmidx, int spinboxidx, int offset)
{
  ilistidx_t  iteridx;
  ilistidx_t  key;
  const char  *intfc;
  int         count;
  nlist_t     *tlist;
  nlist_t     *llist;
  const char  *currintfc;
  const char  *currintfcnm = NULL;

  tlist = nlistAlloc ("cu-i-list", LIST_UNORDERED, NULL);
  llist = nlistAlloc ("cu-i-list-l", LIST_UNORDERED, NULL);

  currintfc = bdjoptGetStr (optidx);
  if (optnmidx != CONFUI_OPT_NONE) {
    currintfcnm = bdjoptGetStr (optnmidx);
  }
  ilistStartIterator (interfaces, &iteridx);
  count = offset;
  gui->uiitem [spinboxidx].listidx = -1;
  while ((key = ilistIterateKey (interfaces, &iteridx)) >= 0) {
    intfc = ilistGetStr (interfaces, key, DYI_LIB);
    if (intfc == NULL) {
      continue;
    }
    if (currintfc != NULL && strcmp (intfc, currintfc) == 0) {
      if (currintfcnm != NULL) {
        const char  *desc;

        desc = ilistGetStr (interfaces, key, DYI_DESC);
        if (strcmp (desc, currintfcnm) == 0) {
          gui->uiitem [spinboxidx].listidx = count;
        }
      } else {
        gui->uiitem [spinboxidx].listidx = count;
      }
    }
    nlistSetStr (tlist, count, ilistGetStr (interfaces, key, DYI_DESC));
    nlistSetStr (llist, count, intfc);
    ++count;
  }
  nlistSort (tlist);
  nlistSort (llist);

  gui->uiitem [spinboxidx].displist = tlist;
  gui->uiitem [spinboxidx].sbkeylist = llist;
}

/* internal routines */

static nlist_t *
confuiGetThemeList (void)
{
  nlist_t     *themelist = NULL;
  slist_t     *sthemelist = NULL;
  slistidx_t  iteridx;
  const char  *nm;
  int         count;
#if BDJ4_USE_GTK3 || BDJ4_USE_GTK4
  char        tbuff [MAXPATHLEN];
  slist_t     *filelist = NULL;
#endif


  logProcBegin ();
  sthemelist = slistAlloc ("cu-themes-s", LIST_ORDERED, NULL);
  themelist = nlistAlloc ("cu-themes", LIST_ORDERED, NULL);

#if BDJ4_USE_GTK3 || BDJ4_USE_GTK4
  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/plocal/share/themes",
        sysvarsGetStr (SV_BDJ4_DIR_MAIN));
    filelist = dirlistRecursiveDirList (tbuff, DIRLIST_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);
    if (slistGetNum (sthemelist, "Windows-10-Acrylic") >= 0) {
      slistSetNum (sthemelist, "Windows-10-Acrylic:dark", 0);
    }
  } else {
    /* for macos */
    filelist = dirlistRecursiveDirList ("/opt/local/share/themes", DIRLIST_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);

    filelist = dirlistRecursiveDirList ("/usr/share/themes", DIRLIST_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);

    snprintf (tbuff, sizeof (tbuff), "%s/.themes", sysvarsGetStr (SV_HOME));
    filelist = dirlistRecursiveDirList (tbuff, DIRLIST_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);
  }
  /* make sure the built-in themes are present */
  slistSetStr (sthemelist, "Adwaita", 0);
  slistSetStr (sthemelist, "Adwaita:dark", 0);
  slistSetStr (sthemelist, "HighContrast", 0);
  slistSetStr (sthemelist, "HighContrastInverse", 0);
#endif   /* gtk3 or gtk4 */

  slistSort (sthemelist);

  slistStartIterator (sthemelist, &iteridx);
  count = 0;
  while ((nm = slistIterateKey (sthemelist, &iteridx)) != NULL) {
    nlistSetStr (themelist, count++, nm);
  }

  slistFree (sthemelist);

  logProcEnd ("");
  return themelist;
}

static slist_t *
confuiGetThemeNames (slist_t *themelist, slist_t *filelist)
{
  slistidx_t    iteridx;
  const char    *fn;
  pathinfo_t    *pi;
#if BDJ4_USE_NULLUI
  static char   *srchdir = "null";
#endif
#if BDJ4_USE_GTK3
  static char   *srchdir = "gtk-3.0";
#endif
#if BDJ4_USE_GTK4
  static char   *srchdir = "gtk-4.0";
#endif
#if BDJ4_USE_GTK3 || BDJ4_USE_GTK4
  static char   *srchfn = "gtk.css";
#endif
#if BDJ4_USE_NULLUI
  static char   *srchfn = "null";
#endif

  logProcBegin ();
  if (filelist == NULL) {
    logProcEnd ("no-filelist");
    return NULL;
  }

  slistStartIterator (filelist, &iteridx);

  /* it would be very messy to try and determine which themes */
  /* already have a dark version listed */

  while ((fn = slistIterateKey (filelist, &iteridx)) != NULL) {
    if (fileopIsDirectory (fn)) {
      pi = pathInfo (fn);
      if (pi->flen == strlen (srchdir) &&
          strncmp (pi->filename, srchdir, strlen (srchdir)) == 0) {
        char  tbuff [MAXPATHLEN];
        char  tmp [MAXPATHLEN];

        snprintf (tmp, sizeof (tmp), "%s/%s", fn, srchfn);
        if (! fileopFileExists (tmp)) {
          continue;
        }

        pi = pathInfo (fn);
        pathInfoGetDir (pi, tbuff, sizeof (tbuff));
        pathInfoFree (pi);

        pi = pathInfo (tbuff);
        strlcpy (tmp, pi->filename, pi->flen + 1);
        slistSetNum (themelist, tmp, 0);
      }
      pathInfoFree (pi);
    } /* is directory */
  } /* for each file */

  logProcEnd ("");
  return themelist;
}

static char *
confuiMakeQRCodeFile (char *title, char *uri)
{
  char          *data;
  char          *ndata;
  char          *qruri;
  char          baseuri [MAXPATHLEN];
  char          tbuff [MAXPATHLEN];
  FILE          *fh;
  size_t        dlen;

  logProcBegin ();
  qruri = mdmalloc (MAXPATHLEN);

  pathbldMakePath (baseuri, sizeof (baseuri),
      "", "", PATHBLD_MP_DIR_TEMPLATE);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", BDJ4_HTML_EXT, PATHBLD_MP_DIR_TEMPLATE);

  data = filedataReadAll (tbuff, &dlen);
  ndata = regexReplaceLiteral (data, "#TITLE#", title);
  mdfree (data);
  data = ndata;
  ndata = regexReplaceLiteral (data, "#BASEURL#", baseuri);
  mdfree (data);
  data = ndata;
  ndata = regexReplaceLiteral (data, "#QRCODEURL#", uri);
  mdfree (data);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", BDJ4_HTML_EXT, PATHBLD_MP_DREL_TMP);
  fh = fileopOpen (tbuff, "w");
  dlen = strlen (ndata);
  fwrite (ndata, dlen, 1, fh);
  mdextfclose (fh);
  fclose (fh);

  snprintf (qruri, MAXPATHLEN, "%s/%s/%s",
      AS_FILE_PFX, sysvarsGetStr (SV_BDJ4_DIR_DATATOP), tbuff);

  mdfree (ndata);
  logProcEnd ("");
  return qruri;
}

static void
confuiUpdateOrgExample (org_t *org, const char *data, uiwcont_t *uiwidgetp)
{
  song_t    *song;
  char      *tdata;
  char      *disp;

  if (data == NULL || org == NULL) {
    return;
  }

  logProcBegin ();
  tdata = mdstrdup (data);
  song = songAlloc ();
  songParse (song, tdata, 0);
  disp = orgMakeSongPath (org, song, NULL);
  pathDisplayPath (disp, strlen (disp));
  uiLabelSetText (uiwidgetp, disp);
  songFree (song);
  mdfree (disp);
  mdfree (tdata);
  logProcEnd ("");
}

int32_t
confuiOrgPathSelect (void *udata, const char *sval)
{
  confuigui_t *gui = udata;

  logProcBegin ();
  if (sval != NULL && *sval) {
    bdjoptSetStr (OPT_G_ORGPATH, sval);
  }
  confuiUpdateOrgExamples (gui, sval);
  logProcEnd ("");
  return UICB_CONT;
}

static bool
confuiSearchDispSel (confuigui_t *gui, int selidx, const char *disp)
{
  dispsel_t     *dispsel;
  slist_t       *sellist;
  slistidx_t    iteridx;
  const char    *tkey;
  bool          found = false;

  dispsel = gui->dispsel;

  sellist = dispselGetList (dispsel, selidx);
  slistStartIterator (sellist, &iteridx);
  while ((tkey = slistIterateKey (sellist, &iteridx)) != NULL) {
    if (strcmp (tkey, disp) == 0) {
      found = true;
      break;
    }
  }

  return found;
}

static void
confuiCreateTagListingMultDisp (confuigui_t *gui, slist_t *dlist,
    int sidx, int eidx)
{
  slist_t       *editlist = NULL;
  const char    *disp;
  slistidx_t    iteridx;

  editlist = slistAlloc ("dyn-edit-tag-list", LIST_ORDERED, NULL);

  slistStartIterator (dlist, &iteridx);
  while ((disp = slistIterateKey (dlist, &iteridx)) != NULL) {
    bool    found;

    /* this is all very inefficient, as the dispsel lists are unordered */
    /* but the lists are relatively short */
    found = false;
    for (int i = sidx; i <= eidx; ++i) {
      found |= confuiSearchDispSel (gui, i, disp);
    }

    if (! found) {
      slistSetNum (editlist, disp, slistGetNum (dlist, disp));
    }
  }

  uiduallistSet (gui->dispselduallist, editlist, DL_LIST_SOURCE);
  slistFree (editlist);
}

