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

static gint uiSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gboolean uiSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata);
static gboolean uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata);
static char * uiSpinboxTextGetDisp (slist_t *list, int idx);

static gboolean uiuitilsSpinboxTextKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata);

void
uiSpinboxTextInit (uispinbox_t *spinbox)
{
  logProcBegin (LOG_PROC, "uiSpinboxTextInit");
  spinbox->spinbox = NULL;
  spinbox->curridx = 0;
  spinbox->textGetProc = NULL;
  spinbox->udata = NULL;
  spinbox->indisp = false;
  spinbox->changed = false;
  spinbox->maxWidth = 0;
  spinbox->list = NULL;
  logProcEnd (LOG_PROC, "uiSpinboxTextInit", "");
}


void
uiSpinboxTextFree (uispinbox_t *spinbox)
{
  ;
}


GtkWidget *
uiSpinboxTextCreate (uispinbox_t *spinbox, void *udata)
{
  logProcBegin (LOG_PROC, "uiSpinboxTextCreate");
  spinbox->spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->spinbox), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->spinbox), TRUE);
  gtk_widget_set_margin_top (spinbox->spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox->spinbox, uiBaseMarginSz);
  g_signal_connect (spinbox->spinbox, "output",
      G_CALLBACK (uiSpinboxTextDisplay), spinbox);
  g_signal_connect (spinbox->spinbox, "input",
      G_CALLBACK (uiSpinboxInput), spinbox);
  spinbox->udata = udata;
  /* for some reason, if the selection background color alone is set, the */
  /* text color temporarily becomes white on light colored themes */
  /* the text color must be set also */
  /* these changes are to make the spinbox read-only */
  uiSetCss (spinbox->spinbox,
      "spinbutton { caret-color: @theme_base_color; } "
      "selection { background-color: @theme_base_color; color: @theme_text_color; }"
      );
  g_signal_connect (spinbox->spinbox, "key-press-event",
      G_CALLBACK (uiuitilsSpinboxTextKeyCallback), NULL);
  logProcEnd (LOG_PROC, "uiSpinboxTextCreate", "");
  return spinbox->spinbox;
}

void
uiSpinboxTextSet (uispinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  logProcBegin (LOG_PROC, "uiSpinboxTextSet");
  uiSpinboxSet (spinbox->spinbox, (double) min, (double) (count - 1));
  /* will width in characters be enough for some glyphs? */
  /* certainly not if languages are mixed */
  spinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (spinbox->spinbox), spinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (spinbox->spinbox), spinbox->maxWidth + 2);
  spinbox->list = list;
  spinbox->keylist = keylist;
  spinbox->textGetProc = textGetProc;
  logProcEnd (LOG_PROC, "uiSpinboxTextSet", "");
}

int
uiSpinboxTextGetIdx (uispinbox_t *spinbox)
{
  int val;

  val = (int) uiSpinboxGetValue (spinbox->spinbox);
  return val;
}

int
uiSpinboxTextGetValue (uispinbox_t *spinbox)
{
  int val;
  int nval;

  val = (int) uiSpinboxGetValue (spinbox->spinbox);
  nval = val;
  if (spinbox->keylist != NULL) {
    nval = nlistGetNum (spinbox->keylist, val);
  }
  return nval;
}

void
uiSpinboxTextSetValue (uispinbox_t *spinbox, int value)
{
  nlistidx_t    idx;

  idx = nlistGetIdx (spinbox->list, value);
  uiSpinboxSetValue (spinbox->spinbox, (double) idx);
}

void
uiSpinboxTimeInit (uispinbox_t *spinbox)
{
  uiSpinboxTextInit (spinbox);
}

void
uiSpinboxTimeFree (uispinbox_t *spinbox)
{
  uiSpinboxTextFree (spinbox);
}

GtkWidget *
uiSpinboxTimeCreate (uispinbox_t *spinbox, void *udata)
{
  logProcBegin (LOG_PROC, "uiSpinboxTimeCreate");
  spinbox->spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->spinbox), 5000.0, 60000.0);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->spinbox), 0.0, 600000.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox->spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox->spinbox, uiBaseMarginSz);
  g_signal_connect (spinbox->spinbox, "output",
      G_CALLBACK (uiSpinboxTimeDisplay), spinbox);
  g_signal_connect (spinbox->spinbox, "input",
      G_CALLBACK (uiSpinboxInput), spinbox);
  spinbox->udata = udata;
  logProcEnd (LOG_PROC, "uiSpinboxTimeCreate", "");
  return spinbox->spinbox;
}

ssize_t
uiSpinboxTimeGetValue (uispinbox_t *spinbox)
{
  ssize_t value;

  value = (ssize_t) uiSpinboxGetValue (spinbox->spinbox);
  return value;
}

void
uiSpinboxTimeSetValue (uispinbox_t *spinbox, ssize_t value)
{
  uiSpinboxSetValue (spinbox->spinbox, (double) value);
}


GtkWidget *
uiSpinboxIntCreate (void)
{
  GtkWidget   *spinbox;

  logProcBegin (LOG_PROC, "uiSpinboxIntCreate");
  spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 1.0, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);
  logProcEnd (LOG_PROC, "uiSpinboxIntCreate", "");
  return spinbox;
}

GtkWidget *
uiSpinboxDoubleCreate (void)
{
  GtkWidget   *spinbox;

  logProcBegin (LOG_PROC, "uiSpinboxDoubleCreate");
  spinbox = gtk_spin_button_new (NULL, 0.0, 1);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);
  logProcEnd (LOG_PROC, "uiSpinboxDoubleCreate", "");
  return spinbox;
}

void
uiSpinboxSet (GtkWidget *spinbox, double min, double max)
{
  logProcBegin (LOG_PROC, "uiSpinboxSet");
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox), min);
  logProcEnd (LOG_PROC, "uiSpinboxSet", "");
}


double
uiSpinboxGetValue (GtkWidget *spinbox)
{
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiSpinboxGetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiSpinboxGetValue", "spinbox-null");
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox));
  value = gtk_adjustment_get_value (adjustment);
  logProcEnd (LOG_PROC, "uiSpinboxGetValue", "");
  return value;
}

void
uiSpinboxSetValue (GtkWidget *spinbox, double value)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "uiSpinboxSetValue");

  if (spinbox == NULL) {
    logProcEnd (LOG_PROC, "uiSpinboxSetValue", "spinbox-null");
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox));
  gtk_adjustment_set_value (adjustment, value);
  logProcEnd (LOG_PROC, "uiSpinboxSetValue", "");
}

bool
uiSpinboxIsChanged (uispinbox_t *spinbox)
{
  if (spinbox == NULL) {
    return false;
  }
  return spinbox->changed;
}

/* internal routines */

/* gtk spinboxes are a bit bizarre */
static gint
uiSpinboxInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uispinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  gdouble           value;

  logProcBegin (LOG_PROC, "uiSpinboxInput");

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->spinbox));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  spinbox->changed = true;
  logProcEnd (LOG_PROC, "uiSpinboxInput", "");
  return TRUE;
}

static gboolean
uiSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  char              *disp;
  double            value;
  char              tbuff [100];

  logProcBegin (LOG_PROC, "uiSpinboxTextDisplay");

  if (spinbox->indisp) {
    logProcEnd (LOG_PROC, "uiSpinboxTextDisplay", "lock-in-disp");
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
    disp = uiSpinboxTextGetDisp (spinbox->list, spinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", spinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (spinbox->spinbox), tbuff);
  spinbox->indisp = false;
  logProcEnd (LOG_PROC, "uiSpinboxTextDisplay", "");
  return TRUE;
}

static gboolean
uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  double            value;
  char              tbuff [100];

  logProcBegin (LOG_PROC, "uiSpinboxTimeDisplay");

  if (spinbox->indisp) {
    logProcEnd (LOG_PROC, "uiSpinboxTimeDisplay", "lock-in-disp");
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
  logProcEnd (LOG_PROC, "uiSpinboxTimeDisplay", "");
  return TRUE;
}

static char *
uiSpinboxTextGetDisp (slist_t *list, int idx)
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

