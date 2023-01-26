/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include <gtk/gtk.h>

#include "bdjstring.h"
#include "colorutils.h"
#include "localeutil.h"
#include "log.h"  // needed for glogwriteroutput
#include "mdebug.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"

static char **cssdata = NULL;
static int  csscount = 0;

static GLogWriterOutput uiGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);
static void uiAddScreenCSS (const char *css);

int uiBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;

void
uiUIInitialize (void)
{
  int argc = 0;

  uiInitUILog ();
  gtk_init (&argc, NULL);
  /* gtk mucks up the locale settings */
  localeInit ();
}

void
uiUIProcessEvents (void)
{
  gtk_main_iteration_do (FALSE);
  while (gtk_events_pending ()) {
    gtk_main_iteration_do (FALSE);
  }
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
uiSetCss (GtkWidget *w, const char *style)
{
  GtkCssProvider  *tcss;
  char            *tstyle;

  tcss = gtk_css_provider_new ();
  tstyle = mdstrdup (style);
  ++csscount;
  cssdata = mdrealloc (cssdata, sizeof (char *) * csscount);
  cssdata [csscount-1] = tstyle;

  gtk_css_provider_load_from_data (tcss, tstyle, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (w),
      GTK_STYLE_PROVIDER (tcss),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void
uiSetUIFont (const char *uifont, const char *accentColor,
    const char *errorColor)
{
  char            tbuff [1024];
  char            wbuff [300];
  char            *p;
  int             sz = 0;

  *tbuff = '\0';

  if (uifont != NULL && *uifont) {
    strlcpy (wbuff, uifont, sizeof (wbuff));
    if (uifont != NULL && *uifont) {
      p = strrchr (wbuff, ' ');
      if (p != NULL) {
        ++p;
        if (isdigit (*p)) {
          --p;
          *p = '\0';
          ++p;
          sz = atoi (p);
        }
      }

      snprintf (tbuff, sizeof (tbuff), "* { font-family: '%s'; } ", wbuff);
    }
  }

  strlcat (tbuff, "entry { color: @theme_fg_color; } ", sizeof (tbuff));
  strlcat (tbuff, "menu { background-color: shade(@theme_base_color,0.7); } ", sizeof (tbuff));
  strlcat (tbuff, "progressbar { background-image: none; } ", sizeof (tbuff));
  strlcat (tbuff, "trough { border-width: 0px; } ", sizeof (tbuff));
  strlcat (tbuff, "trough progress { border-width: 0px; } ", sizeof (tbuff));

  if (accentColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "label." ACCENT_CLASS " { color: %s; } ", accentColor);
    strlcat (tbuff, wbuff, sizeof (tbuff));
    snprintf (wbuff, sizeof (wbuff),
        "label." DARKACCENT_CLASS " { color: shade(%s,0.7); } ", accentColor);
    strlcat (tbuff, wbuff, sizeof (tbuff));
    snprintf (wbuff, sizeof (wbuff),
        "entry." ACCENT_CLASS " { color: %s; } ", accentColor);
    strlcat (tbuff, wbuff, sizeof (tbuff));
  }
  if (errorColor != NULL) {
    snprintf (wbuff, sizeof (wbuff),
        "label." ERROR_CLASS " { color: %s; } ", errorColor);
    strlcat (tbuff, wbuff, sizeof (tbuff));
  }

  if (sz > 0) {
    snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; } ", sz);
    strlcat (tbuff, wbuff, sizeof (tbuff));
    sz -= 2;
    snprintf (wbuff, sizeof (wbuff), " menuitem label { font-size: %dpt; } ", sz);
    strlcat (tbuff, wbuff, sizeof (tbuff));
    snprintf (wbuff, sizeof (wbuff), " .confnotebook tab label { font-size: %dpt; } ", sz);
    strlcat (tbuff, wbuff, sizeof (tbuff));
  }

  uiAddScreenCSS (tbuff);
}

void
uiAddColorClass (const char *classnm, const char *color)
{
  char            tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s { color: %s; } ", classnm, color);
  uiAddScreenCSS (tbuff);
}

void
uiAddBGColorClass (const char *classnm, const char *color)
{
  char            tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "%s { background-color: %s; } ", classnm, color);
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

  p = mdstrdup (css);
  ++csscount;
  cssdata = mdrealloc (cssdata, sizeof (char *) * csscount);
  cssdata [csscount-1] = p;

  tcss = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (tcss, p, -1, NULL);
  screen = gdk_screen_get_default ();
  if (screen != NULL) {
    gtk_style_context_add_provider_for_screen (screen,
        GTK_STYLE_PROVIDER (tcss),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
}

