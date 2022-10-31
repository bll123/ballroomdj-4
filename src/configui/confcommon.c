#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "dirlist.h"
#include "dispsel.h"
#include "dnctypes.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "inline.h"
#include "log.h"
#include "nlist.h"
#include "orgutil.h"
#include "pathbld.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "validate.h"
#include "webclient.h"

static nlist_t * confuiGetThemeList (void);
static slist_t * confuiGetThemeNames (slist_t *themelist, slist_t *filelist);
static char * confuiGetLocalIP (confuigui_t *gui);
static char * confuiMakeQRCodeFile (char *title, char *uri);
static void confuiUpdateOrgExample (org_t *org, char *data, UIWidget *uiwidgetp);


/* the theme list is used by both the ui and the marquee selections */
void
confuiLoadThemeList (confuigui_t *gui)
{
  nlist_t     *tlist;
  nlistidx_t  iteridx;
  int         count;
  bool        usesys = false;
  char        *p;

  p = bdjoptGetStr (OPT_MP_UI_THEME);
  /* use the system default if the ui theme is empty */
  if (p == NULL || ! *p) {
    usesys = true;
  }

  tlist = confuiGetThemeList ();
  nlistStartIterator (tlist, &iteridx);
  count = 0;
  while ((p = nlistIterateValueData (tlist, &iteridx)) != NULL) {
    if (strcmp (p, bdjoptGetStr (OPT_MP_MQ_THEME)) == 0) {
      gui->uiitem [CONFUI_SPINBOX_MQ_THEME].listidx = count;
    }
    if (! usesys &&
        strcmp (p, bdjoptGetStr (OPT_MP_UI_THEME)) == 0) {
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
  char          *tag;
  const char    *valstr;
  bdjmobilemq_t type;
  UIWidget      *uiwidgetp = NULL;

  logProcBegin (LOG_PROC, "confuiUpdateMobmqQrcode");

  type = (bdjmobilemq_t) bdjoptGetNum (OPT_P_MOBILEMARQUEE);

  confuiSetStatusMsg (gui, "");
  if (type == MOBILEMQ_OFF) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (type == MOBILEMQ_INTERNET) {
    tag = bdjoptGetStr (OPT_P_MOBILEMQTAG);
    valstr = validate (tag, VAL_NOT_EMPTY | VAL_NO_SPACES);
    if (valstr != NULL) {
      /* CONTEXT: configuration: mobile marquee: the name to use for internet routing */
      snprintf (tbuff, sizeof (tbuff), valstr, _("Name"));
      confuiSetStatusMsg (gui, tbuff);
    }
    snprintf (uridisp, sizeof (uridisp), "%s%s?v=1&tag=%s",
        sysvarsGetStr (SV_HOST_MOBMQ), sysvarsGetStr (SV_URI_MOBMQ),
        tag);
  }
  if (type == MOBILEMQ_LOCAL) {
    char *ip;

    ip = confuiGetLocalIP (gui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%"PRId64,
        ip, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  }

  if (type != MOBILEMQ_OFF) {
    /* CONTEXT: configuration: qr code: title display for mobile marquee */
    qruri = confuiMakeQRCodeFile (_("Mobile Marquee"), uridisp);
  }

  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uiwidget;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  dataFree (gui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uri);
  gui->uiitem [CONFUI_WIDGET_MMQ_QR_CODE].uri = strdup (qruri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateMobmqQrcode", "");
}

void
confuiUpdateRemctrlQrcode (confuigui_t *gui)
{
  char          uridisp [MAXPATHLEN];
  char          *qruri = "";
  char          tbuff [MAXPATHLEN];
  long          onoff;
  UIWidget      *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiUpdateRemctrlQrcode");

  onoff = (bdjmobilemq_t) bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (onoff == 0) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (onoff == 1) {
    char *ip;

    ip = confuiGetLocalIP (gui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%"PRId64,
        ip, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (onoff == 1) {
    /* CONTEXT: configuration: qr code: title display for mobile remote control */
    qruri = confuiMakeQRCodeFile (_("Mobile Remote Control"), uridisp);
  }

  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uiwidget;
  uiLinkSet (uiwidgetp, uridisp, qruri);
  dataFree (gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri);
  gui->uiitem [CONFUI_WIDGET_RC_QR_CODE].uri = strdup (qruri);
  if (*qruri) {
    free (qruri);
  }
  logProcEnd (LOG_PROC, "confuiUpdateRemctrlQrcode", "");
}

void
confuiUpdateOrgExamples (confuigui_t *gui, char *pathfmt)
{
  char      *data;
  org_t     *org;
  UIWidget  *uiwidgetp;

  if (pathfmt == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "confuiUpdateOrgExamples");
  org = orgAlloc (pathfmt);
  assert (org != NULL);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..1\nALBUM\n..Smooth\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Cha Cha\nGENRE\n..Ballroom Dance\nTITLE\n..Smooth\n";
  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_1].uiwidget;
  confuiUpdateOrgExample (org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 4: Latin Eyes\nALBUMARTIST\n..WRD\nARTIST\n..Gizelle D'Cole\nDANCE\n..Rumba\nGENRE\n..Ballroom Dance\nTITLE\n..Asi\n";
  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_2].uiwidget;
  confuiUpdateOrgExample (org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..1\nTRACKNUMBER\n..3\nALBUM\n..Shaman\nALBUMARTIST\n..Santana\nARTIST\n..Santana\nDANCE\n..Waltz\nTITLE\n..The Game of Love\nGENRE\n..Latin";
  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_3].uiwidget;
  confuiUpdateOrgExample (org, data, uiwidgetp);

  data = "FILE\n..none\nDISC\n..2\nTRACKNUMBER\n..2\nALBUM\n..The Ultimate Latin Album 9: Footloose\nALBUMARTIST\n..\nARTIST\n..Raphael\nDANCE\n..Rumba\nTITLE\n..Ni tú ni yo\nGENRE\n..Latin";
  uiwidgetp = &gui->uiitem [CONFUI_WIDGET_AO_EXAMPLE_4].uiwidget;
  confuiUpdateOrgExample (org, data, uiwidgetp);

  orgFree (org);
  logProcEnd (LOG_PROC, "confuiUpdateOrgExamples", "");
}

void
confuiSetStatusMsg (confuigui_t *gui, const char *msg)
{
  uiLabelSetText (&gui->statusMsg, msg);
}

void
confuiSelectFileDialog (confuigui_t *gui, int widx, char *startpath,
    char *mimefiltername, char *mimetype)
{
  char        *fn = NULL;
  uiselect_t  *selectdata;

  logProcBegin (LOG_PROC, "confuiSelectFileDialog");
  selectdata = uiDialogCreateSelect (&gui->window,
      /* CONTEXT: configuration: file selection dialog: window title */
      _("Select File"), startpath, NULL, mimefiltername, mimetype);
  fn = uiSelectFileDialog (selectdata);
  if (fn != NULL) {
    uiEntrySetValue (gui->uiitem [widx].entry, fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected loc: %s", fn);
    free (fn);
  }
  free (selectdata);
  logProcEnd (LOG_PROC, "confuiSelectFileDialog", "");
}

void
confuiCreateTagListingDisp (confuigui_t *gui)
{
  dispselsel_t  selidx;

  logProcBegin (LOG_PROC, "confuiCreateTagListingDisp");

  selidx = uiSpinboxTextGetValue (gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);

  if (selidx == DISP_SEL_SONGEDIT_A ||
      selidx == DISP_SEL_SONGEDIT_B ||
      selidx == DISP_SEL_SONGEDIT_C) {
    uiduallistSet (gui->dispselduallist, gui->edittaglist,
        DUALLIST_TREE_SOURCE);
  } else {
    uiduallistSet (gui->dispselduallist, gui->listingtaglist,
        DUALLIST_TREE_SOURCE);
  }
  logProcEnd (LOG_PROC, "confuiCreateTagListingDisp", "");
}

void
confuiCreateTagSelectedDisp (confuigui_t *gui)
{
  dispselsel_t  selidx;
  slist_t       *sellist;
  dispsel_t     *dispsel;

  logProcBegin (LOG_PROC, "confuiCreateTagSelectedDisp");


  selidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);

  dispsel = gui->dispsel;
  sellist = dispselGetList (dispsel, selidx);

  uiduallistSet (gui->dispselduallist, sellist, DUALLIST_TREE_TARGET);
  logProcEnd (LOG_PROC, "confuiCreateTagSelectedDisp", "");
}

/* internal routines */

static nlist_t *
confuiGetThemeList (void)
{
  slist_t     *filelist = NULL;
  nlist_t     *themelist = NULL;
  char        tbuff [MAXPATHLEN];
  slist_t     *sthemelist = NULL;
  slistidx_t  iteridx;
  char        *nm;
  int         count;


  logProcBegin (LOG_PROC, "confuiGetThemeList");
  sthemelist = slistAlloc ("cu-themes-s", LIST_ORDERED, NULL);
  themelist = nlistAlloc ("cu-themes", LIST_ORDERED, free);

  if (isWindows ()) {
    snprintf (tbuff, sizeof (tbuff), "%s/plocal/share/themes",
        sysvarsGetStr (SV_BDJ4MAINDIR));
    filelist = dirlistRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);
  } else {
    /* for macos */
    filelist = dirlistRecursiveDirList ("/opt/local/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);

    filelist = dirlistRecursiveDirList ("/usr/share/themes", FILEMANIP_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);

    snprintf (tbuff, sizeof (tbuff), "%s/.themes", sysvarsGetStr (SV_HOME));
    filelist = dirlistRecursiveDirList (tbuff, FILEMANIP_DIRS);
    confuiGetThemeNames (sthemelist, filelist);
    slistFree (filelist);
  }
  /* make sure the built-in themes are present */
  slistSetStr (sthemelist, "Adwaita", 0);
  /* adwaita-dark does not appear to work on macos w/macports */
  if (! isMacOS()) {
    slistSetStr (sthemelist, "Adwaita-dark", 0);
  }
  slistSetStr (sthemelist, "HighContrast", 0);
  slistSetStr (sthemelist, "HighContrastInverse", 0);
  slistSort (sthemelist);

  slistStartIterator (sthemelist, &iteridx);
  count = 0;
  while ((nm = slistIterateKey (sthemelist, &iteridx)) != NULL) {
    nlistSetStr (themelist, count++, nm);
  }

  logProcEnd (LOG_PROC, "confuiGetThemeList", "");
  return themelist;
}

static slist_t *
confuiGetThemeNames (slist_t *themelist, slist_t *filelist)
{
  slistidx_t    iteridx;
  char          *fn;
  pathinfo_t    *pi;
  static char   *srchdir = "gtk-3.0";
  char          tbuff [MAXPATHLEN];
  char          tmp [MAXPATHLEN];

  logProcBegin (LOG_PROC, "confuiGetThemeNames");
  if (filelist == NULL) {
    logProcEnd (LOG_PROC, "confuiGetThemeNames", "no-filelist");
    return NULL;
  }

  slistStartIterator (filelist, &iteridx);

  /* the key value used here is meaningless */
  while ((fn = slistIterateKey (filelist, &iteridx)) != NULL) {
    if (fileopIsDirectory (fn)) {
      pi = pathInfo (fn);
      if (pi->flen == strlen (srchdir) &&
          strncmp (pi->filename, srchdir, strlen (srchdir)) == 0) {
        strlcpy (tbuff, pi->dirname, pi->dlen + 1);
        pathInfoFree (pi);
        pi = pathInfo (tbuff);
        strlcpy (tmp, pi->filename, pi->flen + 1);
        slistSetStr (themelist, tmp, 0);
      }
      pathInfoFree (pi);
    } /* is directory */
  } /* for each file */

  logProcEnd (LOG_PROC, "confuiGetThemeNames", "");
  return themelist;
}

static char *
confuiGetLocalIP (confuigui_t *gui)
{
  char    *ip;

  if (gui->localip == NULL) {
    ip = webclientGetLocalIP ();
    gui->localip = strdup (ip);
    free (ip);
  }

  return gui->localip;
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

  logProcBegin (LOG_PROC, "confuiMakeQRCodeFile");
  qruri = malloc (MAXPATHLEN);

  pathbldMakePath (baseuri, sizeof (baseuri),
      "", "", PATHBLD_MP_TEMPLATEDIR);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", ".html", PATHBLD_MP_TEMPLATEDIR);

  /* this is gross */
  data = filedataReadAll (tbuff, &dlen);
  ndata = filedataReplace (data, &dlen, "#TITLE#", title);
  free (data);
  data = ndata;
  ndata = filedataReplace (data, &dlen, "#BASEURL#", baseuri);
  free (data);
  data = ndata;
  ndata = filedataReplace (data, &dlen, "#QRCODEURL#", uri);
  free (data);

  pathbldMakePath (tbuff, sizeof (tbuff),
      "qrcode", ".html", PATHBLD_MP_TMPDIR);
  fh = fopen (tbuff, "w");
  fwrite (ndata, dlen, 1, fh);
  fclose (fh);
  /* windows requires an extra slash in front, and it doesn't hurt on linux */
  snprintf (qruri, MAXPATHLEN, "file:///%s/%s",
      sysvarsGetStr (SV_BDJ4DATATOPDIR), tbuff);

  free (ndata);
  logProcEnd (LOG_PROC, "confuiMakeQRCodeFile", "");
  return qruri;
}

static void
confuiUpdateOrgExample (org_t *org, char *data, UIWidget *uiwidgetp)
{
  song_t    *song;
  char      *tdata;
  char      *disp;

  if (data == NULL || org == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "confuiUpdateOrgExample");
  tdata = strdup (data);
  song = songAlloc ();
  assert (song != NULL);
  songParse (song, tdata, 0);
  disp = orgMakeSongPath (org, song);
  if (isWindows ()) {
    pathWinPath (disp, strlen (disp));
  }
  uiLabelSetText (uiwidgetp, disp);
  songFree (song);
  free (disp);
  free (tdata);
  logProcEnd (LOG_PROC, "confuiUpdateOrgExample", "");
}

bool
confuiOrgPathSelect (void *udata, long idx)
{
  confuigui_t *gui = udata;
  char        *sval = NULL;
  int         widx;

  logProcBegin (LOG_PROC, "confuiOrgPathSelect");
  widx = CONFUI_COMBOBOX_AO_PATHFMT;
  sval = slistGetDataByIdx (gui->uiitem [widx].displist, idx);
  gui->uiitem [widx].listidx = idx;
  if (sval != NULL && *sval) {
    bdjoptSetStr (OPT_G_AO_PATHFMT, sval);
  }
  confuiUpdateOrgExamples (gui, sval);
  logProcEnd (LOG_PROC, "confuiOrgPathSelect", "");
  return UICB_CONT;
}

