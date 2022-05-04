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
uiutilsUIInitialize (void)
{
  int argc = 0;
  uiutilsInitUILog ();
  gtk_init (&argc, NULL);
}

void
uiutilsUIProcessEvents (void)
{
  while (gtk_events_pending ()) {
    gtk_main_iteration_do (FALSE);
  }
}

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
  if (uifont == NULL || ! *uifont) {
    return;
  }

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
    snprintf (tbuff, sizeof (tbuff), "* { font-family: '%s'; } ", wbuff);
    if (sz > 0) {
      snprintf (wbuff, sizeof (wbuff), " * { font-size: %dpt; } ", sz);
      strlcat (tbuff, wbuff, MAXPATHLEN);
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

inline void
uiutilsInitUILog (void)
{
  g_log_set_writer_func (uiutilsGtkLogger, NULL, NULL);
}

GtkWidget *
uiutilsCreateCheckButton (const char *txt, int value)
{
  GtkWidget   *widget;

  logProcBegin (LOG_PROC, "uiutilsCreateCheckButton");

  widget = gtk_check_button_new_with_label (txt);
  assert (widget != NULL);
  gtk_widget_set_margin_top (widget, uiutilsBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiutilsBaseMarginSz);
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

inline void
uiutilsWidgetDisable (GtkWidget *widget)
{
  gtk_widget_set_sensitive (widget, FALSE);
}

inline void
uiutilsWidgetEnable (GtkWidget *widget)
{
  gtk_widget_set_sensitive (widget, TRUE);
}

inline void
uiutilsWidgetExpandHoriz (GtkWidget *widget)
{
  gtk_widget_set_hexpand (widget, TRUE);
}

inline void
uiutilsWidgetExpandVert (GtkWidget *widget)
{
  gtk_widget_set_vexpand (widget, TRUE);
}

inline void
uiutilsWidgetSetAllMargins (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_top (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
  gtk_widget_set_margin_start (widget, margin);
  gtk_widget_set_margin_end (widget, margin);
}

void
uiutilsWidgetSetMarginTop (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_top (widget, margin);
}

void
uiutilsWidgetSetMarginStart (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_start (widget, margin);
}


inline void
uiutilsWidgetAlignHorizFill (GtkWidget *widget)
{
  gtk_widget_set_halign (widget, GTK_ALIGN_FILL);
}

inline void
uiutilsWidgetAlignHorizStart (GtkWidget *widget)
{
  gtk_widget_set_halign (widget, GTK_ALIGN_START);
}

inline void
uiutilsWidgetAlignHorizEnd (GtkWidget *widget)
{
  gtk_widget_set_halign (widget, GTK_ALIGN_END);
}

inline void
uiutilsWidgetAlignVertFill (GtkWidget *widget)
{
  gtk_widget_set_valign (widget, GTK_ALIGN_FILL);
}

inline void
uiutilsWidgetAlignVertStart (GtkWidget *widget)
{
  gtk_widget_set_valign (widget, GTK_ALIGN_START);
}

inline void
uiutilsWidgetDisableFocus (GtkWidget *widget)
{
  gtk_widget_set_can_focus (widget, FALSE);
}

inline void
uiutilsWidgetHide (GtkWidget *widget)
{
  gtk_widget_hide (widget);
}

inline void
uiutilsWidgetShow (GtkWidget *widget)
{
  gtk_widget_show (widget);
}

inline void
uiutilsWidgetShowAll (GtkWidget *widget)
{
  gtk_widget_show_all (widget);
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

