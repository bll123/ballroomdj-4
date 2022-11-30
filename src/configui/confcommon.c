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
#include "log.h"
#include "nlist.h"
#include "orgutil.h"
#include "pathbld.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "templateutil.h"
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
  bool          enabled;
  UIWidget      *uiwidgetp = NULL;

  logProcBegin (LOG_PROC, "confuiUpdateMobmqQrcode");

  enabled = bdjoptGetNum (OPT_P_MOBILEMARQUEE);

  confuiSetStatusMsg (gui, "");
  if (! enabled) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (enabled) {
    char *ip;

    ip = confuiGetLocalIP (gui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%"PRId64,
        ip, bdjoptGetNum (OPT_P_MOBILEMQPORT));
  }

  if (enabled) {
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
  bool          enabled;
  UIWidget      *uiwidgetp;

  logProcBegin (LOG_PROC, "confuiUpdateRemctrlQrcode");

  enabled = bdjoptGetNum (OPT_P_REMOTECONTROL);

  if (! enabled) {
    *tbuff = '\0';
    *uridisp = '\0';
    qruri = "";
  }
  if (enabled) {
    char *ip;

    ip = confuiGetLocalIP (gui);
    snprintf (uridisp, sizeof (uridisp), "http://%s:%"PRId64,
        ip, bdjoptGetNum (OPT_P_REMCONTROLPORT));
  }

  if (enabled) {
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
confuiUpdateOrgExamples (confuigui_t *gui, char *orgpath)
{
  char      *data;
  org_t     *org;
  UIWidget  *uiwidgetp;

  if (orgpath == NULL) {
    return;
  }

  logProcBegin (LOG_PROC, "confuiUpdateOrgExamples");
  org = orgAlloc (orgpath);
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

void
confuiPopulateOptions (confuigui_t *gui)
{
  const char  *sval;
  ssize_t     nval;
  nlistidx_t  selidx;
  double      dval;
  confuibasetype_t basetype;
  confuiouttype_t outtype;
  char        tbuff [MAXPATHLEN];
  GdkRGBA     gcolor;
  long        debug = 0;
  bool        accentcolorchanged = false;
  bool        profilecolorchanged = false;
  bool        localechanged = false;
  bool        themechanged = false;

  logProcBegin (LOG_PROC, "confuiPopulateOptions");
  for (int i = 0; i < CONFUI_ITEM_MAX; ++i) {
    int     musicq = 0;
    bool    isqueueitem = false;

    sval = "fail";
    nval = -1;
    dval = -1.0;

    basetype = gui->uiitem [i].basetype;
    outtype = gui->uiitem [i].outtype;

    switch (basetype) {
      case CONFUI_NONE: {
        break;
      }
      case CONFUI_ENTRY: {
        sval = uiEntryGetValue (gui->uiitem [i].entry);
        break;
      }
      case CONFUI_SPINBOX_TEXT: {
        nval = uiSpinboxTextGetValue (gui->uiitem [i].spinbox);
        if (outtype == CONFUI_OUT_STR) {
          if (gui->uiitem [i].sbkeylist != NULL) {
            sval = nlistGetStr (gui->uiitem [i].sbkeylist, nval);
          } else {
            sval = nlistGetStr (gui->uiitem [i].displist, nval);
          }
        }
        break;
      }
      case CONFUI_SPINBOX_NUM: {
        nval = (ssize_t) uiSpinboxGetValue (&gui->uiitem [i].uiwidget);
        break;
      }
      case CONFUI_SPINBOX_DOUBLE: {
        dval = uiSpinboxGetValue (&gui->uiitem [i].uiwidget);
        nval = (ssize_t) (dval * 1000.0);
        outtype = CONFUI_OUT_NUM;
        break;
      }
      case CONFUI_SPINBOX_TIME: {
        nval = (ssize_t) uiSpinboxTimeGetValue (gui->uiitem [i].spinbox);
        if (i == CONFUI_SPINBOX_STOP_AT_TIME) {
          /* convert to hh:mm */
          nval *= 60;
        }
        break;
      }
      case CONFUI_COLOR: {
        gtk_color_chooser_get_rgba (
            GTK_COLOR_CHOOSER (gui->uiitem [i].widget), &gcolor);
        snprintf (tbuff, sizeof (tbuff), "#%02x%02x%02x",
            (int) round (gcolor.red * 255.0),
            (int) round (gcolor.green * 255.0),
            (int) round (gcolor.blue * 255.0));
        sval = tbuff;
        break;
      }
      case CONFUI_FONT: {
        sval = gtk_font_chooser_get_font (
            GTK_FONT_CHOOSER (gui->uiitem [i].widget));
        break;
      }
      case CONFUI_SWITCH: {
        nval = uiSwitchGetValue (gui->uiitem [i].uiswitch);
        break;
      }
      case CONFUI_CHECK_BUTTON: {
        nval = uiToggleButtonIsActive (&gui->uiitem [i].uiwidget);
        break;
      }
      case CONFUI_COMBOBOX: {
        sval = slistGetDataByIdx (gui->uiitem [i].displist,
            gui->uiitem [i].listidx);
        outtype = CONFUI_OUT_STR;
        break;
      }
    }

    if (i == CONFUI_SPINBOX_AUDIO_OUTPUT) {
      uispinbox_t  *spinbox;

      spinbox = gui->uiitem [i].spinbox;
      if (! uiSpinboxIsChanged (spinbox)) {
        continue;
      }
    }

    if (gui->uiitem [i].bdjoptIdx >= OPT_Q_ACTIVE) {
      musicq = gui->uiitem [CONFUI_SPINBOX_PLAYER_QUEUE].listidx;
      isqueueitem = true;
    }

    switch (outtype) {
      case CONFUI_OUT_NONE: {
        break;
      }
      case CONFUI_OUT_STR: {
        if (i == CONFUI_SPINBOX_LOCALE) {
          if (strcmp (sysvarsGetStr (SV_LOCALE), sval) != 0) {
            localechanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_ACCENT_COLOR) {
          if (strcmp (bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sval) != 0) {
            accentcolorchanged = true;
          }
        }
        if (i == CONFUI_WIDGET_UI_PROFILE_COLOR) {
          if (strcmp (bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sval) != 0) {
            profilecolorchanged = true;
          }
        }
        if (i == CONFUI_SPINBOX_UI_THEME) {
          if (strcmp (bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sval) != 0) {
            themechanged = true;
          }
        }
        if (isqueueitem) {
          bdjoptSetStrPerQueue (gui->uiitem [i].bdjoptIdx, sval, musicq);
        } else {
          bdjoptSetStr (gui->uiitem [i].bdjoptIdx, sval);
        }
        break;
      }
      case CONFUI_OUT_NUM: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, nval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_DOUBLE: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, dval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, dval);
        }
        break;
      }
      case CONFUI_OUT_BOOL: {
        if (isqueueitem) {
          bdjoptSetNumPerQueue (gui->uiitem [i].bdjoptIdx, nval, musicq);
        } else {
          bdjoptSetNum (gui->uiitem [i].bdjoptIdx, nval);
        }
        break;
      }
      case CONFUI_OUT_CB: {
        nlistSetNum (gui->filterDisplaySel,
            nlistGetNum (gui->filterLookup, i), nval);
        break;
      }
      case CONFUI_OUT_DEBUG: {
        if (nval) {
          long  idx;
          long  bitval;

          idx = i - CONFUI_WIDGET_DEBUG_1;
          bitval = 1 << idx;
          debug |= bitval;
        }
        break;
      }
    } /* out type */

    if (i == CONFUI_SPINBOX_LOCALE &&
        localechanged) {
      sysvarsSetStr (SV_LOCALE, sval);
      snprintf (tbuff, sizeof (tbuff), "%.2s", sval);
      sysvarsSetStr (SV_LOCALE_SHORT, tbuff);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "locale", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      fileopDelete (tbuff);

      /* if the set locale does not match the system or default locale */
      /* save it in the locale file */
      if (strcmp (sval, sysvarsGetStr (SV_LOCALE_SYSTEM)) != 0 &&
          strcmp (sval, "en_GB") != 0) {
        FILE    *fh;

        fh = fopen (tbuff, "w");
        fprintf (fh, "%s\n", sval);
        fclose (fh);
      }
    }

    if (i == CONFUI_ENTRY_MUSIC_DIR) {
      strlcpy (tbuff, bdjoptGetStr (gui->uiitem [i].bdjoptIdx), sizeof (tbuff));
      pathNormPath (tbuff, sizeof (tbuff));
      bdjoptSetStr (gui->uiitem [i].bdjoptIdx, tbuff);
    }

    if (i == CONFUI_SPINBOX_UI_THEME && themechanged) {
      FILE    *fh;

      sval = bdjoptGetStr (gui->uiitem [i].bdjoptIdx);
      pathbldMakePath (tbuff, sizeof (tbuff),
          "theme", BDJ4_CONFIG_EXT, PATHBLD_MP_DATA);
      /* if the theme name is the same as the current default theme */
      /* don't write out the theme file.  want to use the default */
      if (strcmp (sval, sysvarsGetStr (SV_THEME_DEFAULT)) != 0) {
        fh = fopen (tbuff, "w");
        if (sval != NULL) {
          fprintf (fh, "%s\n", sval);
        }
        fclose (fh);
      } else {
        fileopDelete (tbuff);
      }
    }

    if (i == CONFUI_WIDGET_UI_ACCENT_COLOR &&
        accentcolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_WIDGET_UI_PROFILE_COLOR &&
        profilecolorchanged) {
      templateImageCopy (sval);
    }

    if (i == CONFUI_SPINBOX_RC_HTML_TEMPLATE) {
      sval = bdjoptGetStr (gui->uiitem [i].bdjoptIdx);
      templateHttpCopy (sval, "bdj4remote.html");
    }
  } /* for each item */

  selidx = uiSpinboxTextGetValue (
      gui->uiitem [CONFUI_SPINBOX_DISP_SEL].spinbox);
  confuiDispSaveTable (gui, selidx);

  bdjoptSetNum (OPT_G_DEBUGLVL, debug);
  logProcEnd (LOG_PROC, "confuiPopulateOptions", "");
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

  nlistFree (sthemelist);

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
  widx = CONFUI_COMBOBOX_ORGPATH;
  sval = slistGetDataByIdx (gui->uiitem [widx].displist, idx);
  gui->uiitem [widx].listidx = idx;
  if (sval != NULL && *sval) {
    bdjoptSetStr (OPT_G_ORGPATH, sval);
  }
  confuiUpdateOrgExamples (gui, sval);
  logProcEnd (LOG_PROC, "confuiOrgPathSelect", "");
  return UICB_CONT;
}

