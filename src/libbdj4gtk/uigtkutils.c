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

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "fileop.h"
#include "ilist.h"
#include "localeutil.h"
#include "log.h"
#include "pathbld.h"
#include "pathutil.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "uiutils.h"

static char **cssdata = NULL;
static int  csscount = 0;

static GLogWriterOutput uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

int uiutilsBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;

void
uiutilsCleanup (void)
{
  if (cssdata != NULL) {
    for (int i = 0; i < csscount; ++i) {
      if (cssdata [i] != NULL) {
        free (cssdata [i]);
      }
    }
    free (cssdata);
    csscount = 0;
    cssdata = NULL;
  }
}

void
uiutilsSetCss (GtkWidget *w, char *style)
{
  GtkCssProvider  *tcss;
  char            *tstyle;

  logProcBegin (LOG_PROC, "uiutilsSetCss");

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
  logProcEnd (LOG_PROC, "uiutilsSetCss", "");
}

void
uiutilsSetUIFont (char *uifont)
{
  GtkCssProvider  *tcss;
  GdkScreen       *screen;
  char            tbuff [MAXPATHLEN];
  char            wbuff [MAXPATHLEN];
  char            *p;
  int             sz = 0;

  logProcBegin (LOG_PROC, "uiutilsSetUIFont");

  strlcpy (wbuff, uifont, MAXPATHLEN);
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
    snprintf (tbuff, sizeof (tbuff), "* { font-family: '%s'; }", wbuff);
    if (sz > 0) {
      snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; }", sz);
      sz -= 2;
      snprintf (wbuff, sizeof (wbuff), " menuitem label { font-size: %dpt; }", sz);
      strlcat (tbuff, wbuff, MAXPATHLEN);
    }

    p = strdup (tbuff);
    ++csscount;
    cssdata = realloc (cssdata, sizeof (char *) * csscount);
    cssdata [csscount-1] = p;

    gtk_css_provider_load_from_data (tcss, p, -1, NULL);
    screen = gdk_screen_get_default ();
    if (screen != NULL) {
      gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (tcss),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
  logProcEnd (LOG_PROC, "uiutilsSetUIFont", "");
}

void
uiutilsInitUILog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

int
uiutilsCreateApplication (int argc, char *argv [],
    char *tag, GtkApplication **app, void *activateFunc, void *udata)
{
  int             status;
  char            tbuff [80];

  logProcBegin (LOG_PROC, "uiutilsCreateApplication");

  snprintf (tbuff, sizeof (tbuff), "org.bdj4.BDJ4.%s", tag);
  *app = gtk_application_new (tbuff, G_APPLICATION_NON_UNIQUE);

  g_signal_connect (*app, "activate", G_CALLBACK (activateFunc), udata);

  /* gtk messes up the locale setting somehow; a re-bind is necessary */
  localeInit ();

  status = g_application_run (G_APPLICATION (*app), argc, argv);

  logProcEnd (LOG_PROC, "uiutilsCreateApplication", "");
  return status;
}

GtkWidget *
uiutilsCreateSwitch (int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateSwitch");

  widget = gtk_switch_new ();
  assert (widget != NULL);
  gtk_switch_set_active (GTK_SWITCH (widget), value);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
  logProcEnd (LOG_PROC, "uiutilsCreateSwitch", "");
  return widget;
}

GtkWidget *
uiutilsCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateCheckButton");

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, 2);
  gtk_widget_set_margin_start (widget, 2);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  logProcEnd (LOG_PROC, "uiutilsCreateCheckButton", "");
  return widget;
}

void
uiutilsGetForegroundColor (GtkWidget *widget, char *buff, size_t sz)
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

void
uiutilsWidgetSetAllMargins (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_top (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
  gtk_widget_set_margin_start (widget, margin);
  gtk_widget_set_margin_end (widget, margin);
}

/* internal routines */

static GLogWriterOutput
uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields,
    gsize n_fields,
    gpointer udata)
{
  char      *msg;

  if (logLevel != G_LOG_LEVEL_DEBUG) {
    msg = g_log_writer_format_fields (logLevel, fields, n_fields, FALSE);
    logMsg (LOG_GTK, LOG_IMPORTANT, msg);
    if (strcmp (sysvarsGetStr (SV_BDJ4_RELEASELEVEL), "alpha") == 0) {
      fprintf (stderr, "%s\n", msg);
    }
  }

  return G_LOG_WRITER_HANDLED;
}

