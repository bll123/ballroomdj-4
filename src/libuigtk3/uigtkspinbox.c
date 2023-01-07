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

#include "bdj4intl.h"
#include "mdebug.h"
#include "tmutil.h"
#include "ui.h"

typedef struct uispinbox {
  int             sbtype;
  UIWidget        uispinbox;
  UICallback      *convcb;
  int             curridx;
  uispinboxdisp_t textGetProc;
  void            *udata;
  int             maxWidth;
  slist_t         *list;
  nlist_t         *keylist;
  nlist_t         *idxlist;
  bool            processing : 1;
  bool            changed : 1;
} uispinbox_t;

static gint uiSpinboxTextInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gint uiSpinboxNumInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gint uiSpinboxDoubleInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gint uiSpinboxTimeInput (GtkSpinButton *sb, gdouble *newval, gpointer udata);
static gboolean uiSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata);
static gboolean uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata);
static char * uiSpinboxTextGetDisp (slist_t *list, int idx);

static gboolean uiuitilsSpinboxTextKeyCallback (GtkWidget *w, GdkEventKey *event, gpointer udata);
static void uiSpinboxValueChangedHandler (GtkSpinButton *sb, gpointer udata);
static gboolean uiSpinboxDoubleDefaultDisplay (GtkSpinButton *sb, gpointer udata);

uispinbox_t *
uiSpinboxInit (void)
{
  uispinbox_t   *spinbox;

  spinbox = mdmalloc (sizeof (uispinbox_t));
  uiutilsUIWidgetInit (&spinbox->uispinbox);
  spinbox->convcb = NULL;
  spinbox->curridx = 0;
  spinbox->textGetProc = NULL;
  spinbox->udata = NULL;
  spinbox->processing = false;
  spinbox->changed = false;
  spinbox->maxWidth = 0;
  spinbox->list = NULL;
  spinbox->keylist = NULL;
  spinbox->idxlist = NULL;
  spinbox->sbtype = SB_TEXT;

  return spinbox;
}


void
uiSpinboxFree (uispinbox_t *spinbox)
{
  if (spinbox != NULL) {
    nlistFree (spinbox->idxlist);
    mdfree (spinbox);
  }
}


void
uiSpinboxTextCreate (uispinbox_t *spinbox, void *udata)
{
  spinbox->uispinbox.widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), TRUE);
  gtk_widget_set_margin_top (spinbox->uispinbox.widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox->uispinbox.widget, uiBaseMarginSz);
  g_signal_connect (spinbox->uispinbox.widget, "output",
      G_CALLBACK (uiSpinboxTextDisplay), spinbox);
  g_signal_connect (spinbox->uispinbox.widget, "input",
      G_CALLBACK (uiSpinboxTextInput), spinbox);
  spinbox->udata = udata;
  /* for some reason, if the selection background color alone is set, the */
  /* text color temporarily becomes white on light colored themes */
  /* the text color must be set also */
  /* these changes are to make the spinbox read-only */
  uiSetCss (spinbox->uispinbox.widget,
      "spinbutton { caret-color: @theme_base_color; } "
      "selection { background-color: @theme_base_color; color: @theme_text_color; }"
      );
  g_signal_connect (spinbox->uispinbox.widget, "key-press-event",
      G_CALLBACK (uiuitilsSpinboxTextKeyCallback), NULL);
}

void
uiSpinboxTextSet (uispinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  /* will width in characters be enough for some glyphs? */
  /* certainly not if languages are mixed */
  spinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (spinbox->uispinbox.widget), spinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (spinbox->uispinbox.widget), spinbox->maxWidth + 2);
  spinbox->list = list;
  spinbox->keylist = keylist;
  if (spinbox->keylist != NULL) {
    nlistidx_t  iteridx;
    nlistidx_t  sbidx;
    nlistidx_t  val;

    spinbox->idxlist = nlistAlloc ("sb-idxlist", LIST_ORDERED, NULL);
    nlistStartIterator (spinbox->keylist, &iteridx);
    while ((sbidx = nlistIterateKey (spinbox->keylist, &iteridx)) >= 0) {
      val = nlistGetNum (spinbox->keylist, sbidx);
      nlistSetNum (spinbox->idxlist, val, sbidx);
    }
  }
  spinbox->textGetProc = textGetProc;
  /* set the range after setting up the list as the range set will */
  /* generate a call to gtk display processing */
  uiSpinboxSet (&spinbox->uispinbox, (double) min, (double) (count - 1));
}

int
uiSpinboxTextGetIdx (uispinbox_t *spinbox)
{
  int val;

  val = (int) uiSpinboxGetValue (&spinbox->uispinbox);
  return val;
}

int
uiSpinboxTextGetValue (uispinbox_t *spinbox)
{
  int nval;

  nval = (int) uiSpinboxGetValue (&spinbox->uispinbox);
  if (spinbox->keylist != NULL) {
    nval = nlistGetNum (spinbox->keylist, nval);
  }
  return nval;
}

void
uiSpinboxTextSetValue (uispinbox_t *spinbox, int value)
{
  nlistidx_t    idx;

  if (spinbox == NULL) {
    return;
  }

  idx = value;
  if (spinbox->idxlist != NULL) {
    idx = nlistGetNum (spinbox->idxlist, value);
  }
  uiSpinboxSetValue (&spinbox->uispinbox, (double) idx);
}

void
uiSpinboxDisable (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return;
  }
  uiWidgetDisable (&uispinbox->uispinbox);
}

void
uiSpinboxEnable (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return;
  }
  uiWidgetEnable (&uispinbox->uispinbox);
}

uispinbox_t *
uiSpinboxTimeInit (int sbtype)
{
  uispinbox_t *uispinbox;

  uispinbox = uiSpinboxInit ();
  uispinbox->sbtype = sbtype;
  return uispinbox;
}

void
uiSpinboxTimeCreate (uispinbox_t *spinbox, void *udata, UICallback *convcb)
{
  double  inca = 5000.0;
  double  incb = 60000.0;

  spinbox->convcb = convcb;
  spinbox->uispinbox.widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox->uispinbox.widget), 1.0);
  if (spinbox->sbtype == SB_TIME_BASIC) {
    inca = 5000.0;
    incb = 60000.0;
  }
  if (spinbox->sbtype == SB_TIME_PRECISE) {
    inca = 100.0;
    incb = 30000.0;
  }
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), inca, incb);
  /* this range is for maximum play time */
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), 0.0, 1200000.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), FALSE);
  gtk_widget_set_margin_top (spinbox->uispinbox.widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox->uispinbox.widget, uiBaseMarginSz);
  g_signal_connect (spinbox->uispinbox.widget, "output",
      G_CALLBACK (uiSpinboxTimeDisplay), spinbox);
  g_signal_connect (spinbox->uispinbox.widget, "input",
      G_CALLBACK (uiSpinboxTimeInput), spinbox);
  spinbox->udata = udata;
  return;
}

ssize_t
uiSpinboxTimeGetValue (uispinbox_t *spinbox)
{
  ssize_t value;

  value = (ssize_t) uiSpinboxGetValue (&spinbox->uispinbox);
  return value;
}

void
uiSpinboxTimeSetValue (uispinbox_t *spinbox, ssize_t value)
{
  uiSpinboxSetValue (&spinbox->uispinbox, (double) value);
}

void
uiSpinboxTextSetValueChangedCallback (uispinbox_t *spinbox, UICallback *uicb)
{
  uiSpinboxSetValueChangedCallback (&spinbox->uispinbox, uicb);
}

void
uiSpinboxTimeSetValueChangedCallback (uispinbox_t *spinbox, UICallback *uicb)
{
  uiSpinboxSetValueChangedCallback (&spinbox->uispinbox, uicb);
}

void
uiSpinboxSetValueChangedCallback (UIWidget *uiwidget, UICallback *uicb)
{
  g_signal_connect (uiwidget->widget, "value-changed",
      G_CALLBACK (uiSpinboxValueChangedHandler), uicb);
}

void
uiSpinboxIntCreate (UIWidget *uiwidget)
{
  GtkWidget   *spinbox;

  spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 1.0, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);
  g_signal_connect (spinbox, "input",
      G_CALLBACK (uiSpinboxNumInput), NULL);
  uiwidget->widget = spinbox;
}

void
uiSpinboxDoubleCreate (UIWidget *uiwidget)
{
  GtkWidget   *spinbox;

  spinbox = gtk_spin_button_new (NULL, 0.0, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);
  g_signal_connect (spinbox, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);
  uiwidget->widget = spinbox;
}

void
uiSpinboxDoubleDefaultCreate (uispinbox_t *spinbox)
{
  GtkWidget   *widget;

  widget = gtk_spin_button_new (NULL, -0.1, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_entry_set_alignment (GTK_ENTRY (widget), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxDoubleDefaultDisplay), spinbox);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);
  spinbox->uispinbox.widget = widget;
}

void
uiSpinboxSetRange (uispinbox_t *spinbox, double min, double max)
{
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->uispinbox.widget),
      min, max);
}


void
uiSpinboxWrap (uispinbox_t *spinbox)
{
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), TRUE);
}

void
uiSpinboxSet (UIWidget *uispinbox, double min, double max)
{
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uispinbox->widget), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uispinbox->widget), min);
}

double
uiSpinboxGetValue (UIWidget *uispinbox)
{
  GtkAdjustment     *adjustment;
  gdouble           value;


  if (uispinbox == NULL) {
    return -1;
  }
  if (uispinbox->widget == NULL) {
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uispinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  return value;
}

void
uiSpinboxSetValue (UIWidget *uispinbox, double value)
{
  GtkAdjustment     *adjustment;

  if (uispinbox == NULL) {
    return;
  }
  if (uispinbox->widget == NULL) {
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uispinbox->widget));
  gtk_adjustment_set_value (adjustment, value);
}


bool
uiSpinboxIsChanged (uispinbox_t *spinbox)
{
  if (spinbox == NULL) {
    return false;
  }
  return spinbox->changed;
}

void
uiSpinboxResetChanged (uispinbox_t *spinbox)
{
  if (spinbox == NULL) {
    return;
  }
  spinbox->changed = false;
}

void
uiSpinboxAlignRight (uispinbox_t *spinbox)
{
  gtk_entry_set_alignment (GTK_ENTRY (spinbox->uispinbox.widget), 1.0);
}

void
uiSpinboxSetColor (uispinbox_t *spinbox, const char *color)
{
  char  tbuff [200];

  snprintf (tbuff, sizeof (tbuff),
      "spinbutton { color: %s; } ", color);
  uiSetCss (spinbox->uispinbox.widget, tbuff);
}

UIWidget *
uiSpinboxGetUIWidget (uispinbox_t *spinbox)
{
  return &spinbox->uispinbox;
}


/* internal routines */

/* gtk spinboxes are a bit bizarre */
static gint
uiSpinboxTextInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uispinbox_t   *spinbox = udata;
  GtkAdjustment *adjustment;
  gdouble       value;

  /* text spinboxes do not allow text entry, so the value from the */
  /* adjustment is correct */

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (sb));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  if (spinbox != NULL) {
    spinbox->changed = true;
  }
  return UICB_CONVERTED;
}

/* gtk spinboxes are definitely bizarre */
static gint
uiSpinboxNumInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  const char    *newtext;

  newtext = gtk_entry_get_text (GTK_ENTRY (sb));
  if (newtext != NULL && *newtext) {
    *newval = (gdouble) atol (newtext);
  }
  return UICB_CONVERTED;
}

static gint
uiSpinboxDoubleInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  const char    *newtext;

  newtext = gtk_entry_get_text (GTK_ENTRY (sb));
  if (newtext != NULL) {
    /* CONTEXT: user interface: default setting display */
    if (strcmp (newtext, _("Default")) == 0) {
      *newval = -0.1;
    } else if (*newtext) {
      *newval = atof (newtext);
    }
  }
  return UICB_CONVERTED;
}

static gint
uiSpinboxTimeInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uispinbox_t   *spinbox = udata;
  GtkAdjustment *adjustment;
  gdouble       value;
  const char    *newtext;
  long          newvalue = -1;

  if (spinbox->processing) {
    return UICB_NO_CONV;
  }
  spinbox->processing = true;

  newtext = gtk_entry_get_text (GTK_ENTRY (spinbox->uispinbox.widget));
  if (spinbox->convcb != NULL) {
    newvalue = uiutilsCallbackStrHandler (spinbox->convcb, newtext);
  } else {
    newvalue = tmutilStrToMS (newtext);
  }

  if (newvalue < 0) {
    spinbox->processing = false;
    return UICB_NO_CONV;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->uispinbox.widget));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  if (newvalue != -1) {
    *newval = (double) newvalue;
  }
  spinbox->changed = true;
  spinbox->processing = false;
  return UICB_CONVERTED;
}

static gboolean
uiSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t  *spinbox = udata;
  GtkAdjustment     *adjustment;
  const char        *disp;
  double            value;
  char              tbuff [100];


  if (spinbox->processing) {
    return UICB_NO_DISP;
  }
  spinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (spinbox->uispinbox.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), value);
  spinbox->curridx = (int) value;
  disp = "";
  if (spinbox->textGetProc != NULL) {
    disp = spinbox->textGetProc (spinbox->udata, spinbox->curridx);
  } else if (spinbox->list != NULL) {
    disp = uiSpinboxTextGetDisp (spinbox->list, spinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", spinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (spinbox->uispinbox.widget), tbuff);
  spinbox->processing = false;
  return UICB_DISPLAYED;
}

static gboolean
uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t   *spinbox = udata;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  if (spinbox->processing) {
    return UICB_NO_DISP;
  }
  spinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->uispinbox.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), value);
  if (spinbox->sbtype == SB_TIME_BASIC) {
    tmutilToMS ((ssize_t) value, tbuff, sizeof (tbuff));
  }
  if (spinbox->sbtype == SB_TIME_PRECISE) {
    tmutilToMSD ((ssize_t) value, tbuff, sizeof (tbuff), 1);
  }
  gtk_entry_set_text (GTK_ENTRY (spinbox->uispinbox.widget), tbuff);
  spinbox->processing = false;
  return UICB_DISPLAYED;
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
  /* the up and down arrows are spinbox increment controls */
  /* page up and down are spinbox increment controls */
  /* tab and left tab are navigation controls */
  if (keyval == GDK_KEY_Up ||
      keyval == GDK_KEY_KP_Up ||
      keyval == GDK_KEY_Down ||
      keyval == GDK_KEY_KP_Down ||
      keyval == GDK_KEY_Page_Up ||
      keyval == GDK_KEY_KP_Page_Up ||
      keyval == GDK_KEY_Page_Down ||
      keyval == GDK_KEY_KP_Page_Down ||
      keyval == GDK_KEY_Tab ||
      keyval == GDK_KEY_KP_Tab ||
      keyval == GDK_KEY_ISO_Left_Tab
      ) {
    return UICB_CONT;
  }
  /* control/meta/super keys should be processed also */
  if ((event->state & GDK_CONTROL_MASK) ||
      (event->state & GDK_META_MASK) ||
      (event->state & GDK_SUPER_MASK)) {
    return UICB_CONT;
  }

  return UICB_STOP;
}

static void
uiSpinboxValueChangedHandler (GtkSpinButton *sb, gpointer udata)
{
  UICallback  *uicb = udata;

  uiutilsCallbackHandler (uicb);
}

static gboolean
uiSpinboxDoubleDefaultDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t   *spinbox = udata;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  if (spinbox == NULL) {
    return UICB_NO_DISP;
  }
  if (spinbox->processing) {
    return UICB_NO_DISP;
  }
  if (spinbox->uispinbox.widget == NULL) {
    return UICB_NO_DISP;
  }
  spinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spinbox->uispinbox.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->uispinbox.widget), value);
  if (value < 0) {
    /* CONTEXT: user interface: default setting display */
    gtk_entry_set_text (GTK_ENTRY (spinbox->uispinbox.widget), _("Default"));
  }
  spinbox->processing = false;
  return UICB_DISPLAYED;
}

