#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "bdjstring.h"
#include "log.h"
#include "portability.h"
#include "uiutils.h"

static GLogWriterOutput uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields, gsize n_fields, gpointer udata);

void
uiutilsSetCss (GtkWidget *w, char *style)
{
  GtkCssProvider        *tcss;

  tcss = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (tcss, style, -1, NULL);
  gtk_style_context_add_provider (
      gtk_widget_get_style_context (GTK_WIDGET (w)),
      GTK_STYLE_PROVIDER (tcss),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
    snprintf (tbuff, MAXPATHLEN, "* { font-family: '%s'; }", wbuff);
    if (sz > 0) {
      snprintf (wbuff, MAXPATHLEN, " * { font-size: %dpt; }", sz);
    }
    strlcat (tbuff, wbuff, MAXPATHLEN);
    gtk_css_provider_load_from_data (tcss, tbuff, -1, NULL);
    screen = gdk_screen_get_default ();
    if (screen != NULL) {
      gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (tcss),
          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  }
}

void
uiutilsInitGtkLog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

/* internal routines */

static GLogWriterOutput
uiutilsGtkLogger (GLogLevelFlags logLevel,
    const GLogField* fields,
    gsize n_fields,
    gpointer udata)
{
  char      *msg;

  msg = g_log_writer_format_fields (logLevel, fields, n_fields, FALSE);
  logMsg (LOG_GTK, LOG_IMPORTANT, msg);
  return G_LOG_WRITER_HANDLED;
}

