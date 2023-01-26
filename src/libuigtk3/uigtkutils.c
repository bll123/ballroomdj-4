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
uiSetUICSS (const char *uifont, const char *accentColor,
    const char *errorColor)
{
  char            tbuff [4096];
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

  /* note that these affect dialogs started by the application */
  strlcat (tbuff, "entry { color: @theme_fg_color; } ", sizeof (tbuff));
  strlcat (tbuff, "menu { background-color: shade(@theme_base_color,0.7); } ", sizeof (tbuff));
  strlcat (tbuff, "scale, scale trough { min-height: 5px; } ", sizeof (tbuff));
  strlcat (tbuff, "separator { min-height: 4px; } ", sizeof (tbuff));
  strlcat (tbuff, "scrollbar, scrollbar slider { min-width: 9px; } ", sizeof (tbuff));

  /* read-only spinbox */
  /* for some reason, if the selection background color alone is set, the */
  /* text color temporarily becomes white on light colored themes */
  /* the text color must be set also */
  /* these changes are to make the spinbox read-only */
  strlcat (tbuff,
      "spinbutton." SPINBOX_READONLY_CLASS " { caret-color: @theme_base_color; } "
      "spinbutton." SPINBOX_READONLY_CLASS " selection { background-color: @theme_base_color; color: @theme_text_color; } ",
      sizeof (tbuff));

  /* progress bars */
  strlcat (tbuff, "progressbar { background-image: none; } ", sizeof (tbuff));
  strlcat (tbuff, "progressbar > trough { border-width: 0px; min-height: 25px; } ", sizeof (tbuff));
  strlcat (tbuff, "progressbar > trough progress { border-width: 0px; min-height: 25px; } ", sizeof (tbuff));

  /* dark textbox */
  strlcat (tbuff,
      "textview." TEXTBOX_DARK_CLASS " text { background-color: shade(@theme_base_color,0.8); } ",
      sizeof (tbuff));

  /* dark tree view */
  strlcat (tbuff,
      "treeview." TREEVIEW_DARK_CLASS " { background-color: shade(@theme_base_color,0.8); } "
      "treeview." TREEVIEW_DARK_CLASS ":selected { background-color: @theme_selected_bg_color; } ",
      sizeof (tbuff));

  /* notebooks */
  // notebook header tabs tab label
  strlcat (tbuff,
      "notebook tab:checked { background-color: shade(@theme_base_color,0.6); } ",
      sizeof (tbuff));

  /* scrolled windows */
  strlcat (tbuff,
      "scrolledwindow undershoot.top, scrolledwindow undershoot.right, "
      "scrolledwindow undershoot.left, scrolledwindow undershoot.bottom "
      "{ background-image: none; outline-width: 0; } ", sizeof (tbuff));

  /* our switch */
  strlcat (tbuff,
      "button." SWITCH_CLASS " { "
      "  border-bottom-left-radius: 15px; "
      "  border-bottom-right-radius: 15px; "
      "  border-top-left-radius: 15px; "
      "  border-top-right-radius: 15px; "
      "  background-color: shade(@theme_base_color,0.8); "
      "  border-color: shade(@theme_base_color,0.8); "
      "} "
      "button.switch:checked { "
      "  background-color: shade(@theme_base_color,0.8); "
      "} "
      "button.switch:hover { "
      "  background-color: shade(@theme_base_color,0.8); "
      "} ",
      sizeof (tbuff));

  /* flat button */
  strlcat (tbuff, "button." FLATBUTTON_CLASS " { outline-width: 0px; "
      "padding: 0px; border-color: @theme_bg_color; "
      "background-color: @theme_bg_color; } ", sizeof (tbuff));
  strlcat (tbuff, "button." FLATBUTTON_CLASS " widget { outline-width: 0px; } ", sizeof (tbuff));

  /* change indicators */
  strlcat (tbuff,
      "label." CHGIND_NORMAL_CLASS " { border-left-style: solid; border-left-width: 4px; "
      " border-left-color: transparent; } ", sizeof (tbuff));
  strlcat (tbuff,
      "label." CHGIND_CHANGED_CLASS " { border-left-style: solid; border-left-width: 4px; "
      " border-left-color: #11ff11; } ", sizeof (tbuff));
  strlcat (tbuff,
      "label." CHGIND_ERROR_CLASS " { border-left-style: solid; border-left-width: 4px; "
      " border-left-color: #ff1111; } ", sizeof (tbuff));

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

    snprintf (wbuff, sizeof (wbuff),
        "progressbar." ACCENT_CLASS " > trough > progress { background-color: %s; }",
        accentColor);
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

  if (strlen (tbuff) >= sizeof (tbuff)) {
    fprintf (stderr, "possible css overflow: %zd\n", strlen (tbuff));
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

