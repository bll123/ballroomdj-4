/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>
#include <hb.h>
#include <hb-glib.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "colorutils.h"
#include "filedata.h"
#include "log.h"          // needed for glogwriteroutput
#include "oslocale.h"
#include "pathbld.h"
#include "mdebug.h"
#include "sysvars.h"
#include "tmutil.h"
#include "uiclass.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"

#define BDJ4_DEBUG_CSS 0

/* as of 2026-2-22, the length is 5600+ */
enum {
  UIUI_MAX_CSS = 8192,
};

static char **cssdata = NULL;
static int  csscount = 0;
static bool initialized = false;
static const char *currcss = NULL;

static GLogWriterOutput uiGtkLogger (GLogLevelFlags logLevel, const GLogField* fields, gsize n_fields, gpointer udata);
static void uiAddScreenCSS (const char *css);
static void uicssParseError (GtkCssProvider* self, GtkCssSection* section, GError* error, gpointer udata);
static char *uiSetRowHighlight (char *currp, char *endp, const char *accentColor, const char *color, const char *classnm, double shadeval, bool is_dark);

int uiBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;
int uiTextDirection = TEXT_DIR_DEFAULT;
static bool inprocess = false;

const char *
uiBackend (void)
{
  return "gtk3";
}

void
uiUIInitialize (int direction)
{
  int   argc = 0;
  int   gtdir = GTK_TEXT_DIR_LTR;

  if (initialized) {
    return;
  }

  uiInitUILog ();

  /* gtk's gtk_get_locale_direction() gets very confused if the locale */
  /* settings are changed before gtk_init() is called. */

  /* the locale has already been set */
  gtk_disable_setlocale ();
  gtk_init (&argc, NULL);

  uiTextDirection = direction;
  if (direction == TEXT_DIR_RTL) {
    gtdir = GTK_TEXT_DIR_RTL;
  }

  gtk_widget_set_default_direction (gtdir);

  initialized = true;
}

void
uiUIProcessEvents (void)
{
  if (inprocess) {
    return;
  }
  inprocess = true;
  gtk_main_iteration_do (FALSE);
  while (gtk_events_pending ()) {
    gtk_main_iteration_do (FALSE);
  }
  inprocess = false;
}

void
uiUIProcessWaitEvents (void)
{
  for (int i = 0; i < 4; ++i) {
    uiUIProcessEvents ();
    mssleep (5);
  }
}

void
uiCleanup (void)
{
  if (cssdata != NULL) {
    for (int i = 0; i < csscount; ++i) {
      dataFree (cssdata [i]);
    }
    mdfree (cssdata);
    csscount = 0;
    cssdata = NULL;
  }
}

void
uiSetUICSS (uisetup_t *uisetup)
{
  char            *cssbuff;
  char            tbuff [BDJ4_PATH_MAX];
  char            wbuff [400];
  char            *p;
  int             sz = 0;
  char            *tp = NULL;
  char            *tend = NULL;
  const char      *cssnm;

  if (uisetup->rowselColor == NULL || ! *uisetup->rowselColor) {
    uisetup->rowselColor = uisetup->accentColor;
  }
  if (uisetup->rowhlColor == NULL || ! *uisetup->rowhlColor) {
    uisetup->rowhlColor = uisetup->accentColor;
  }

  cssbuff = mdmalloc (UIUI_MAX_CSS);
  *cssbuff = '\0';
  tp = cssbuff;
  tend = cssbuff + UIUI_MAX_CSS;

  /* append the main css */
  pathbldMakePath (tbuff, sizeof (tbuff),
      GTK_CSS_STATIC_FN, BDJ4_CSS_EXT, PATHBLD_MP_DIR_TEMPLATE);
  p = filedataReadAll (tbuff, NULL);
  if (p != NULL) {
    tp = stpecpy (tp, tend, p);
    mdfree (p);
  }

  cssnm = GTK_CSS_LIGHT_FN;
  if (uisetup->is_dark) {
    cssnm = GTK_CSS_DARK_FN;
  }
  pathbldMakePath (tbuff, sizeof (tbuff),
      cssnm, BDJ4_CSS_EXT, PATHBLD_MP_DIR_TEMPLATE);
  p = filedataReadAll (tbuff, NULL);

  /* append the light/dark specific css */
  if (p != NULL) {
    tp = stpecpy (tp, tend, p);
    mdfree (p);
  }

  if (uisetup->uifont != NULL && *uisetup->uifont) {
    char  tmp [100];

    stpecpy (tmp, tmp + sizeof (tmp), uisetup->uifont);
    p = strrchr (tmp, ' ');
    if (p != NULL) {
      ++p;
      if (isdigit (*p)) {
        --p;
        *p = '\0';
        ++p;
        sz = atoi (p);
      }
    }

    snprintf (wbuff, sizeof (wbuff), "* { font-family: '%s'; }\n", tmp);
    tp = stpecpy (tp, tend, wbuff);
  }

  if (uisetup->listingfont != NULL && *uisetup->listingfont) {
    char  tmp [100];
    int   listingsz = 0;

    stpecpy (tmp, tmp + sizeof (tmp), uisetup->listingfont);
    p = strrchr (tmp, ' ');
    if (p != NULL) {
      ++p;
      if (isdigit (*p)) {
        --p;
        *p = '\0';
        ++p;
        listingsz = atoi (p);
      }
    }

    if (listingsz > 0) {
      /* the largest size w/o messing up the display too much */
      int   favsz = listingsz + 1;
      int   headsz = listingsz + 1;

      if (headsz > sz) {
        headsz = sz;
      }

      snprintf (wbuff, sizeof (wbuff),
          ".%s { font-family: '%s'; font-size: %dpt; }\n",
          LISTING_CLASS, tmp, listingsz);
      tp = stpecpy (tp, tend, wbuff);

      snprintf (wbuff, sizeof (wbuff),
          "label.%s { font-size: %dpt; font-weight: bold; }\n",
          LIST_FAV_CLASS, favsz);
      tp = stpecpy (tp, tend, wbuff);

      snprintf (wbuff, sizeof (wbuff),
          "label.%s { font-size: %dpt; font-weight: bold; }\n",
          LIST_HEAD_CLASS, headsz);
      tp = stpecpy (tp, tend, wbuff);
    }
  }

  if (sz > 0) {
    int   tsz;

    snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; }\n", sz);
    tp = stpecpy (tp, tend, wbuff);

    tsz = sz - 2;
    snprintf (wbuff, sizeof (wbuff), " menuitem label { font-size: %dpt; }\n",
        tsz);
    tp = stpecpy (tp, tend, wbuff);

    tsz = sz - 1;
    snprintf (wbuff, sizeof (wbuff), " button.%s label { font-size: %dpt; }\n",
        LEFT_NB_CLASS, tsz);
    tp = stpecpy (tp, tend, wbuff);

    tsz = sz - 3;
    if (uisetup->accentColor != NULL) {
      snprintf (wbuff, sizeof (wbuff),
          " button.bdj-spd-reset label { font-size: %dpt; color: %s; }\n",
          tsz, uisetup->accentColor);
    } else {
      snprintf (wbuff, sizeof (wbuff),
          " button.bdj-spd-reset label { font-size: %dpt; }\n",
          tsz);
    }
    tp = stpecpy (tp, tend, wbuff);
  }

  if (uisetup->accentColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "label.%s { color: %s; }\n", ACCENT_CLASS, uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);
    snprintf (wbuff, sizeof (wbuff),
        "label.%s { color: %s; font-weight: bold; }\n",
        BOLD_ACCENT_CLASS, uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);
    snprintf (wbuff, sizeof (wbuff),
        "label.%s { color: shade(%s,0.8); }\n", DARKACCENT_CLASS,
        uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);

    /* entry foreground color */
    snprintf (wbuff, sizeof (wbuff),
        "entry.%s { color: %s; }\n", ACCENT_CLASS, uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);

    snprintf (wbuff, sizeof (wbuff),
        "progressbar.%s > trough > progress { background-color: %s; }\n",
        ACCENT_CLASS, uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);

    snprintf (wbuff, sizeof (wbuff),
        "menu separator { background-color: shade(%s,0.8); }\n",
        uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);

    snprintf (wbuff, sizeof (wbuff),
        "paned.%s > separator { background-color: %s; padding-bottom: 0px; }\n",
        ACCENT_CLASS, uisetup->accentColor);
    tp = stpecpy (tp, tend, wbuff);
  }

  if (uisetup->rowselColor != NULL) {
    tp = uiSetRowHighlight (tp, tend,
        uisetup->accentColor, uisetup->rowselColor,
        SELECTED_CLASS, 0.15, uisetup->is_dark);
  }

  if (uisetup->rowhlColor != NULL) {
    tp = uiSetRowHighlight (tp, tend,
        uisetup->accentColor, uisetup->rowhlColor,
        ROW_HL_CLASS, 0.05, uisetup->is_dark);
  }

  if (uisetup->errorColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "label.%s { color: %s; }\n", ERROR_CLASS, uisetup->errorColor);
    tp = stpecpy (tp, tend, wbuff);
  }

  if (uisetup->markColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "label.%s { color: %s; }\n", MARK_CLASS, uisetup->markColor);
    tp = stpecpy (tp, tend, wbuff);
  }

  if (uisetup->mqbgColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "window.%s { background-color: %s; }\n", MQ_WIN_CLASS, uisetup->mqbgColor);
    tp = stpecpy (tp, tend, wbuff);
  }

  /* append any user CSS */
  pathbldMakePath (tbuff, sizeof (tbuff),
      GTK_CSS_USER_FN, BDJ4_CSS_EXT, PATHBLD_MP_DREL_DATA);
  p = filedataReadAll (tbuff, NULL);
  if (p != NULL) {
    tp = stpecpy (tp, tend, p);
  }

  /* as of 2026-2-22, the length is 5600+ */
  if (strlen (cssbuff) >= UIUI_MAX_CSS) {
    fprintf (stderr, "ERR: CSS overflow: %zd\n", strlen (cssbuff));
  }

#if BDJ4_DEBUG_CSS
  unlink ("css.txt");
#endif
  uiAddScreenCSS (cssbuff);
  mdfree (cssbuff);
}

void
uiAddColorClass (const char *classnm, const char *color)
{
  char  tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s { color: %s; }\n", classnm, color);
  uiAddScreenCSS (tbuff);
}

void
uiAddBGColorClass (const char *classnm, const char *color)
{
  char  tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s { background-color: %s; }\n", classnm, color);
  uiAddScreenCSS (tbuff);
}

void
uiAddProgressbarClass (const char *classnm, const char *color)
{
  char  tbuff [100];

  snprintf (tbuff, sizeof (tbuff),
      "progressbar.%s > trough > progress { background-color: %s; }\n",
      classnm, color);
  uiAddScreenCSS (tbuff);
}

void
uiInitUILog (void)
{
  g_log_set_writer_func (uiGtkLogger, NULL, NULL);
}

/* internal routines */

static GLogWriterOutput
uiGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields,
    gsize n_fields,
    gpointer udata)
{
  char  *msg;

  if (logLevel != G_LOG_LEVEL_DEBUG) {
    msg = g_log_writer_format_fields (logLevel, fields, n_fields, FALSE);
    mdextalloc (msg);
    logMsg (LOG_GTK, LOG_IMPORTANT, "%s", msg);
    if (strcmp (sysvarsGetStr (SV_BDJ4_DEVELOPMENT), "dev") == 0) {
      fprintf (stderr, "%s\n", msg);
    }
    mdfree (msg); // allocated by glib
  }

  return G_LOG_WRITER_HANDLED;
}

static void
uiAddScreenCSS (const char *css)
{
  GtkCssProvider  *tcss;
  GdkScreen       *screen;
  char            *p;

  for (int i = 0; i < csscount; ++i) {
    if (strcmp (cssdata [i], css) == 0) {
      return;
    }
  }

  p = mdstrdup (css);
  ++csscount;
  cssdata = mdrealloc (cssdata, sizeof (char *) * csscount);
  cssdata [csscount-1] = p;

  tcss = gtk_css_provider_new ();
  currcss = p;
  g_signal_connect (G_OBJECT (tcss), "parsing-error",
      G_CALLBACK (uicssParseError), NULL);
  gtk_css_provider_load_from_data (tcss, p, -1, NULL);
  screen = gdk_screen_get_default ();
  if (screen != NULL) {
    gtk_style_context_add_provider_for_screen (screen,
        GTK_STYLE_PROVIDER (tcss),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
#if BDJ4_DEBUG_CSS
  {
    FILE    *fh;

    fh = fopen ("css.txt", "a");
    fprintf (fh, "%s", css);
    fclose (fh);
  }
#endif
}

/* internal routines */

static void
uicssParseError (GtkCssProvider* self, GtkCssSection* section,
    GError* error, gpointer udata)
{
  int                 s, sp, e, ep;
  GtkCssSectionType   t;

  s = gtk_css_section_get_start_line (section);
  sp = gtk_css_section_get_start_position (section);
  e = gtk_css_section_get_end_line (section);
  ep = gtk_css_section_get_end_position (section);
  t = gtk_css_section_get_section_type (section);
  fprintf (stderr, "ERR: CSS parse: %s from %d/%d to %d/%d type: %d\n", currcss, s, sp, e, ep, t);
  fprintf (stderr, "ERR: CSS parse: %s\n", error->message);
}

static char *
uiSetRowHighlight (char *tp, char *tend,
    const char *accentColor, const char *color,
    const char *classnm, double shadeval, bool is_dark)
{
  char    tmpcolor [40];
  char    wbuff [400];

  if (color == accentColor) {
    /* gtk must have the radix as a . character */
    /* do a little math to force this */
    snprintf (tmpcolor, sizeof (tmpcolor), "mix(@theme_bg_color,%s,%d.%02d)",
        color, (int) shadeval, (int) ((shadeval - (int) shadeval) * 100.0));
  } else {
    stpecpy (tmpcolor, tmpcolor + sizeof (tmpcolor), color);
  }

  /* the problem with using the standard selected-bg-color with virtlist */
  /* is it matches the radio button and check button colors */
  /* (in the matcha series of themes) and */
  /* there is no easy way that i can find to switch the radio buttons and */
  /* check buttons to the obverse colors */
  snprintf (wbuff, sizeof (wbuff),
      "box.horizontal.%s { background-color: %s; }\n",
      classnm, tmpcolor);
  tp = stpecpy (tp, tend, wbuff);

  snprintf (wbuff, sizeof (wbuff),
      "spinbutton.%s, "
      "spinbutton.%s entry, "
      "spinbutton.%s button "
      "{ background-color: %s; }\n",
      classnm, classnm, classnm, tmpcolor);
  tp = stpecpy (tp, tend, wbuff);

  snprintf (wbuff, sizeof (wbuff),
      "entry.%s "
      "{ background-color: %s; }\n",
      classnm, tmpcolor);
  tp = stpecpy (tp, tend, wbuff);

  return tp;
}

void
uiwcontUIInit (uiwcont_t *uiwidget)
{
  return;
}

void
uiwcontUIWidgetInit (uiwcont_t *uiwidget)
{
  return;
}

void
uiwcontUIFree (uiwcont_t *uiwidget)
{
  return;
}

