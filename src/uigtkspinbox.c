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

#include "log.h"
#include "uiutils.h"

static gint uiutilsSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gboolean uiutilsSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata);
static gboolean uiutilsSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata);
static char * uiutilsSpinboxTextGetDisp (slist_t *list, int idx);

static gboolean uiuitilsSpinboxTextKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata);

void
uiutilsSpinboxTextInit (uiutilsspinbox_t *spinbox)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxTextInit");
  spinbox->spinbox = NULL;
  spinbox->curridx = 0;
  spinbox->textGetProc = NULL;
  spinbox->udata = NULL;
  spinbox->indisp = false;
  spinbox->changed = false;
  spinbox->maxWidth = 0;
  spinbox->list = NULL;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTextInit", "");
}


void
uiutilsSpinboxTextFree (uiutilsspinbox_t *spinbox)
{
  ;
}


GtkWidget *
uiutilsSpinboxTextCreate (uiutilsspinbox_t *spinbox, void *udata)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxTextCreate");
  spinbox->spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->spinbox), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->spinbox), TRUE);
  gtk_widget_set_margin_top (spinbox->spinbox, 2);
  gtk_widget_set_margin_start (spinbox->spinbox, 2);
  g_signal_connect (spinbox->spinbox, "output",
      G_CALLBACK (uiutilsSpinboxTextDisplay), spinbox);
  g_signal_connect (spinbox->spinbox, "input",
      G_CALLBACK (uiutilsSpinboxInput), spinbox);
  spinbox->udata = udata;
  /* for some reason, if the selection background color alone is set, the */
  /* text color temporarily becomes white on light colored themes */
  /* the text color must be set also */
  uiutilsSetCss (spinbox->spinbox,
      "spinbutton { caret-color: @theme_base_color; } "
      "selection { background-color: @theme_base_color; color: @theme_text_color; }"
      );
  g_signal_connect (spinbox->spinbox, "key-press-event",
      G_CALLBACK (uiuitilsSpinboxTextKeyCallback), NULL);
  logProcEnd (LOG_PROC, "uiutilsSpinboxTextCreate", "");
  return spinbox->spinbox;
}

void
uiutilsSpinboxTextSet (uiutilsspinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, uiutilsspinboxdisp_t textGetProc)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxTextSet");
  uiutilsSpinboxSet (spinbox->spinbox, (double) min, (double) (count - 1));
  /* will width in characters be enough for some glyphs? */
  /* certainly not if languages are mixed */
  spinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (spinbox->spinbox), spinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (spinbox->spinbox), spinbox->maxWidth + 2);
  spinbox->list = list;
  spinbox->textGetProc = textGetProc;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTextSet", "");
}

int
uiutilsSpinboxTextGetValue (uiutilsspinbox_t *spinbox)
{
  int val;

  val = (int) uiutilsSpinboxGetValue (spinbox->spinbox);
  return val;
}

void
uiutilsSpinboxTextSetValue (uiutilsspinbox_t *spinbox, int value)
{
  uiutilsSpinboxSetValue (spinbox->spinbox, (double) value);
}

void
uiutilsSpinboxTimeInit (uiutilsspinbox_t *spinbox)
{
  uiutilsSpinboxTextInit (spinbox);
}

void
uiutilsSpinboxTimeFree (uiutilsspinbox_t *spinbox)
{
  uiutilsSpinboxTextFree (spinbox);
}

GtkWidget *
uiutilsSpinboxTimeCreate (uiutilsspinbox_t *spinbox, void *udata)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxTimeCreate");
  spinbox->spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->spinbox), 5000.0, 60000.0);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->spinbox), 0.0, 600000.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox->spinbox, 2);
  gtk_widget_set_margin_start (spinbox->spinbox, 2);
  g_signal_connect (spinbox->spinbox, "output",
      G_CALLBACK (uiutilsSpinboxTimeDisplay), spinbox);
  g_signal_connect (spinbox->spinbox, "input",
      G_CALLBACK (uiutilsSpinboxInput), spinbox);
  spinbox->udata = udata;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTimeCreate", "");
  return spinbox->spinbox;
}

ssize_t
uiutilsSpinboxTimeGetValue (uiutilsspinbox_t *spinbox)
{
  ssize_t value;

  value = (ssize_t) uiutilsSpinboxGetValue (spinbox->spinbox);
  return value;
}

void
uiutilsSpinboxTimeSetValue (uiutilsspinbox_t *spinbox, ssize_t value)
{
  uiutilsSpinboxSetValue (spinbox->spinbox, (double) value);
}


GtkWidget *
uiutilsSpinboxIntCreate (void)
{
  GtkWidget   *spinbox;

  logProcBegin (LOG_PROC, "uiutilsSpinboxIntCreate");
  spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 1.0, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, 2);
  gtk_widget_set_margin_start (spinbox, 2);
  logProcEnd (LOG_PROC, "uiutilsSpinboxIntCreate", "");
  return spinbox;
}

GtkWidget *
uiutilsSpinboxDoubleCreate (void)
{
  GtkWidget   *spinbox;

  logProcBegin (LOG_PROC, "uiutilsSpinboxDoubleCreate");
  spinbox = gtk_spin_button_new (NULL, 0.0, 1);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, 2);
  gtk_widget_set_margin_start (spinbox, 2);
  logProcEnd (LOG_PROC, "uiutilsSpinboxDoubleCreate", "");
  return spinbox;
}

void
uiutilsSpinboxSet (GtkWidget *spinbox, double min, double max)
{
  logProcBegin (LOG_PROC, "uiutilsSpinboxSet");
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox), min);
  logProcEnd (LOG_PROC, "uiutilsSpinboxSet", "");
}


double
uiutilsSpinboxGetValue (GtkWidget *spinbox)
{
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiutilsSpinboxGetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxGetValue", "spinbox-null");
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox));
  value = gtk_adjustment_get_value (adjustment);
  logProcEnd (LOG_PROC, "uiutilsSpinboxGetValue", "");
  return value;
}

void
uiutilsSpinboxSetValue (GtkWidget *spinbox, double value)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "uiutilsSpinboxSetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxSetValue", "spinbox-null");
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox));
  gtk_adjustment_set_value (adjustment, value);
  logProcEnd (LOG_PROC, "uiutilsSpinboxSetValue", "");
}

bool
uiutilsSpinboxIsChanged (uiutilsspinbox_t *spinbox)
{
  if (spinbox == NULL) {
    return false;
  }
  return spinbox->changed;
}

/* internal routines */

/* gtk spinboxes are a bit bizarre */
static gint
uiutilsSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uiutilsspinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiutilsSpinboxInput");

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  spinbox->changed = true;
  logProcEnd (LOG_PROC, "uiutilsSpinboxInput", "");
  return TRUE;
}

static gboolean
uiutilsSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata)
{
  uiutilsspinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  char              *disp;
  double            value;
  char              tbuff [100];

  logProcBegin (LOG_PROC, "uiutilsSpinboxTextDisplay");

  if (spinbox->indisp) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxTextDisplay", "lock-in-disp");
    return FALSE;
  }
  spinbox->indisp = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->spinbox), value);
  spinbox->curridx = (int) value;
  disp = "";
  if (spinbox->textGetProc != NULL) {
    disp = spinbox->textGetProc (spinbox->udata, spinbox->curridx);
  } else if (spinbox->list != NULL) {
    disp = uiutilsSpinboxTextGetDisp (spinbox->list, spinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", spinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (spinbox->spinbox), tbuff);
  spinbox->indisp = false;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTextDisplay", "");
  return TRUE;
}

static gboolean
uiutilsSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata)
{
  uiutilsspinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  double            value;
  char              tbuff [100];

  logProcBegin (LOG_PROC, "uiutilsSpinboxTimeDisplay");

  if (spinbox->indisp) {
    logProcEnd (LOG_PROC, "uiutilsSpinboxTimeDisplay", "lock-in-disp");
    return FALSE;
  }
  spinbox->indisp = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->spinbox), value);
  tmutilToMS ((ssize_t) value, tbuff, sizeof (tbuff));
  gtk_entry_set_text (GTK_ENTRY (spinbox->spinbox), tbuff);
  spinbox->indisp = false;
  logProcEnd (LOG_PROC, "uiutilsSpinboxTimeDisplay", "");
  return TRUE;
}

static char *
uiutilsSpinboxTextGetDisp (slist_t *list, int idx)
{
  return nlistGetDataByIdx (list, idx);
}

static gboolean
uiuitilsSpinboxTextKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata)
{
  guint    keyval;

  gdk_event_get_keyval ((GdkEvent *) event, &keyval);
  if (keyval == GDK_KEY_Up ||
      keyval == GDK_KEY_Down ||
      keyval == GDK_KEY_Page_Up ||
      keyval == GDK_KEY_Page_Down) {
    return FALSE;
  }

  return TRUE;
}

