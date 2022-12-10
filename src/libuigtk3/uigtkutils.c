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
#include "localeutil.h"
#include "log.h"  // needed for glogwriteroutput
#include "sysvars.h"
#include "ui.h"

static char **cssdata = NULL;
static int  csscount = 0;

static GLogWriterOutput uiGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

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
uiCleanup (void)
{
  if (cssdata != NULL) {
    for (int i = 0; i < csscount; ++i) {
      dataFree (cssdata [i]);
    }
    free (cssdata);
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
  tstyle = strdup (style);
  ++csscount;
  cssdata = realloc (cssdata, sizeof (char *) * csscount);
  cssdata [csscount-1] = tstyle;

  gtk_css_provider_load_from_data (tcss, tstyle, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (w),
      GTK_STYLE_PROVIDER (tcss),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void
uiSetUIFont (char *uifont)
{
  GtkCssProvider  *tcss;
  GdkScreen       *screen;
  char            tbuff [300];
  char            wbuff [300];
  char            *p;
  int             sz = 12;

  if (uifont == NULL || ! *uifont) {
    return;
  }

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

    tcss = gtk_css_provider_new ();
    snprintf (tbuff, sizeof (tbuff), "* { font-family: '%s'; } ", wbuff);
    if (sz > 0) {
      snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; } ", sz);
      strlcat (tbuff, wbuff, sizeof (tbuff));
      sz -= 2;
      snprintf (wbuff, sizeof (wbuff), " menuitem label { font-size: %dpt; } ", sz);
      strlcat (tbuff, wbuff, sizeof (tbuff));
      snprintf (wbuff, sizeof (wbuff), " .confnotebook tab label { font-size: %dpt; } ", sz);
      strlcat (tbuff, wbuff, sizeof (tbuff));
    }

    p = strdup (tbuff);
    ++csscount;
    cssdata = realloc (cssdata, sizeof (char *) * csscount);
    cssdata [csscount-1] = p;

    gtk_css_provider_load_from_data (tcss, p, -1, NULL);
    screen = gdk_screen_get_default ();
    if (screen != NULL) {
      gtk_style_context_add_provider_for_screen (screen,
          GTK_STYLE_PROVIDER (tcss),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
}

void
uiInitUILog (void)
{
  g_log_set_writer_func (uiGtkLogger, NULL, NULL);
}

void
uiGetForegroundColor (UIWidget *uiwidget, char *buff, size_t sz)
{
  GdkRGBA         gcolor;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (uiwidget->widget);
  gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &gcolor);
  snprintf (buff, sz, "#%02x%02x%02x",
      (int) round (gcolor.red * 255.0),
      (int) round (gcolor.green * 255.0),
      (int) round (gcolor.blue * 255.0));
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
    logMsg (LOG_GTK, LOG_IMPORTANT, "%s", msg);
    if (strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "alpha") == 0) {
      fprintf (stderr, "%s\n", msg);
    }
    free (msg);
  }

  return G_LOG_WRITER_HANDLED;
}

/* these routines will be removed at a later date */

void
uiGetForegroundColorW (GtkWidget *widget, char *buff, size_t sz)
{
  GdkRGBA         gcolor;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &gcolor);
  snprintf (buff, sz, "#%02x%02x%02x",
      (int) round (gcolor.red * 255.0),
      (int) round (gcolor.green * 255.0),
      (int) round (gcolor.blue * 255.0));
}

