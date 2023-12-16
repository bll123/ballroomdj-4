/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
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

#include "bdj4intl.h"
#include "callback.h"
#include "mdebug.h"
#include "tmutil.h"
#include "uiclass.h"
#include "uiwcont.h"

#include "ui-gtk3.h"

#include "ui/uiwcont-int.h"

#include "ui/uikeys.h"
#include "ui/uiui.h"
#include "ui/uiwidget.h"
#include "ui/uispinbox.h"

typedef struct uispinbox {
  int             sbtype;
  uiwcont_t       *spinbox;
  callback_t      *convcb;
  int             curridx;
  uispinboxdisp_t textGetProc;
  void            *udata;
  int             maxWidth;
  slist_t         *list;
  nlist_t         *keylist;
  nlist_t         *idxlist;
  callback_t      *presscb;
  uiwcont_t       *uikey;
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

static bool uiSpinboxTextKeyCallback (void *udata);
static void uiSpinboxValueChangedHandler (GtkSpinButton *sb, gpointer udata);
static gboolean uiSpinboxDoubleDefaultDisplay (GtkSpinButton *sb, gpointer udata);

uispinbox_t *
uiSpinboxInit (void)
{
  uispinbox_t   *uispinbox;

  uispinbox = mdmalloc (sizeof (uispinbox_t));
  uispinbox->spinbox = NULL;
  uispinbox->convcb = NULL;
  uispinbox->curridx = 0;
  uispinbox->textGetProc = NULL;
  uispinbox->udata = NULL;
  uispinbox->processing = false;
  uispinbox->changed = false;
  uispinbox->maxWidth = 0;
  uispinbox->list = NULL;
  uispinbox->keylist = NULL;
  uispinbox->idxlist = NULL;
  uispinbox->sbtype = SB_TEXT;
  uispinbox->uikey = uiKeyAlloc ();
  uispinbox->presscb = callbackInit (&uiSpinboxTextKeyCallback,
      uispinbox, NULL);
  return uispinbox;
}


void
uiSpinboxFree (uispinbox_t *uispinbox)
{
  if (uispinbox != NULL) {
    uiwcontFree (uispinbox->spinbox);
    callbackFree (uispinbox->presscb);
    nlistFree (uispinbox->idxlist);
    uiwcontFree (uispinbox->uikey);
    mdfree (uispinbox);
  }
}


void
uiSpinboxTextCreate (uispinbox_t *uispinbox, void *udata)
{
  GtkWidget *widget;

  widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTextDisplay), uispinbox);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTextInput), uispinbox);
  uispinbox->udata = udata;

  uispinbox->spinbox = uiwcontAlloc ();
  uispinbox->spinbox->wbasetype = WCONT_T_SPINBOX;
  uispinbox->spinbox->wtype = WCONT_T_SPINBOX_TEXT;
  uispinbox->spinbox->widget = widget;
  uiKeySetKeyCallback (uispinbox->uikey, uispinbox->spinbox, uispinbox->presscb);

  uiWidgetSetClass (uispinbox->spinbox, SPINBOX_READONLY_CLASS);
}

void
uiSpinboxTextSet (uispinbox_t *uispinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  uispinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (uispinbox->spinbox->widget), uispinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (uispinbox->spinbox->widget), uispinbox->maxWidth + 2);
  uispinbox->list = list;
  uispinbox->keylist = keylist;
  if (uispinbox->keylist != NULL) {
    nlistidx_t  iteridx;
    nlistidx_t  sbidx;
    nlistidx_t  val;

    uispinbox->idxlist = nlistAlloc ("sb-idxlist", LIST_ORDERED, NULL);
    nlistStartIterator (uispinbox->keylist, &iteridx);
    while ((sbidx = nlistIterateKey (uispinbox->keylist, &iteridx)) >= 0) {
      val = nlistGetNum (uispinbox->keylist, sbidx);
      nlistSetNum (uispinbox->idxlist, val, sbidx);
    }
  }
  uispinbox->textGetProc = textGetProc;
  /* set the range after setting up the list as the range set will */
  /* generate a call to gtk display processing */
  uiSpinboxSet (uispinbox->spinbox, (double) min, (double) (count - 1));
}

int
uiSpinboxTextGetIdx (uispinbox_t *uispinbox)
{
  int val;

  val = (int) uiSpinboxGetValue (uispinbox->spinbox);
  return val;
}

int
uiSpinboxTextGetValue (uispinbox_t *uispinbox)
{
  int nval;

  nval = (int) uiSpinboxGetValue (uispinbox->spinbox);
  if (uispinbox->keylist != NULL) {
    nval = nlistGetNum (uispinbox->keylist, nval);
  }
  return nval;
}

void
uiSpinboxTextSetValue (uispinbox_t *uispinbox, int value)
{
  nlistidx_t    idx;

  if (uispinbox == NULL) {
    return;
  }

  idx = value;
  if (uispinbox->idxlist != NULL) {
    idx = nlistGetNum (uispinbox->idxlist, value);
  }
  uiSpinboxSetValue (uispinbox->spinbox, (double) idx);
}

void
uiSpinboxSetState (uispinbox_t *uispinbox, int state)
{
  if (uispinbox == NULL) {
    return;
  }
  uiWidgetSetState (uispinbox->spinbox, state);
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
uiSpinboxTimeCreate (uispinbox_t *uispinbox, void *udata, callback_t *convcb)
{
  double    inca = 5000.0;
  double    incb = 60000.0;
  GtkWidget *widget;

  uispinbox->convcb = convcb;
  widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_entry_set_alignment (GTK_ENTRY (widget), 1.0);
  if (uispinbox->sbtype == SB_TIME_BASIC) {
    inca = 5000.0;
    incb = 60000.0;
  }
  if (uispinbox->sbtype == SB_TIME_PRECISE) {
    inca = 100.0;
    incb = 30000.0;
  }
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), inca, incb);
  /* this range is for maximum play time */
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (widget), 0.0, 1200000.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTimeDisplay), uispinbox);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTimeInput), uispinbox);
  uispinbox->udata = udata;

  uispinbox->spinbox = uiwcontAlloc ();
  uispinbox->spinbox->wbasetype = WCONT_T_SPINBOX;
  uispinbox->spinbox->wtype = WCONT_T_SPINBOX_TIME;
  uispinbox->spinbox->widget = widget;
  return;
}

ssize_t
uiSpinboxTimeGetValue (uispinbox_t *uispinbox)
{
  ssize_t value;

  value = (ssize_t) uiSpinboxGetValue (uispinbox->spinbox);
  return value;
}

void
uiSpinboxTimeSetValue (uispinbox_t *uispinbox, ssize_t value)
{
  uiSpinboxSetValue (uispinbox->spinbox, (double) value);
}

void
uiSpinboxTextSetValueChangedCallback (uispinbox_t *uispinbox, callback_t *uicb)
{
  uiSpinboxSetValueChangedCallback (uispinbox->spinbox, uicb);
}

void
uiSpinboxTimeSetValueChangedCallback (uispinbox_t *uispinbox, callback_t *uicb)
{
  uiSpinboxSetValueChangedCallback (uispinbox->spinbox, uicb);
}

void
uiSpinboxSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  g_signal_connect (uiwidget->widget, "value-changed",
      G_CALLBACK (uiSpinboxValueChangedHandler), uicb);
}

uiwcont_t *
uiSpinboxIntCreate (void)
{
  uiwcont_t   *uiwidget;
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

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX_NUM;
  uiwidget->widget = spinbox;
  return uiwidget;
}

uiwcont_t *
uiSpinboxDoubleCreate (void)
{
  uiwcont_t   *uiwidget;
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

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX_DOUBLE;
  uiwidget->widget = spinbox;
  return uiwidget;
}

void
uiSpinboxDoubleDefaultCreate (uispinbox_t *uispinbox)
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
      G_CALLBACK (uiSpinboxDoubleDefaultDisplay), uispinbox);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);

  uispinbox->spinbox = uiwcontAlloc ();
  uispinbox->spinbox->wbasetype = WCONT_T_SPINBOX;
  uispinbox->spinbox->wtype = WCONT_T_SPINBOX_DOUBLE_DFLT;
  uispinbox->spinbox->widget = widget;
}

void
uiSpinboxSetRange (uispinbox_t *uispinbox, double min, double max)
{
  if (uispinbox == NULL || uispinbox->spinbox == NULL ||
      uispinbox->spinbox->widget == NULL) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uispinbox->spinbox->widget),
      min, max);
}

void
uiSpinboxSetIncrement (uiwcont_t *spinbox, double incr, double pageincr)
{
  if (spinbox == NULL ||
      spinbox->widget == NULL) {
    return;
  }

  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox->widget),
      incr, pageincr);
}


void
uiSpinboxWrap (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL || uispinbox->spinbox == NULL ||
      uispinbox->spinbox->widget == NULL) {
    return;
  }

  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (uispinbox->spinbox->widget), TRUE);
}

void
uiSpinboxSet (uiwcont_t *spinbox, double min, double max)
{
  if (spinbox == NULL || spinbox->widget == NULL) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbox->widget), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbox->widget), min);
}

double
uiSpinboxGetValue (uiwcont_t *spinbox)
{
  GtkAdjustment     *adjustment;
  gdouble           value;


  if (spinbox == NULL) {
    return -1;
  }
  if (spinbox->widget == NULL) {
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (spinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  return value;
}

void
uiSpinboxSetValue (uiwcont_t *spinbox, double value)
{
  GtkAdjustment     *adjustment;

  if (spinbox == NULL) {
    return;
  }
  if (spinbox->widget == NULL) {
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (spinbox->widget));
  gtk_adjustment_set_value (adjustment, value);
}


bool
uiSpinboxIsChanged (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return false;
  }
  return uispinbox->changed;
}

void
uiSpinboxResetChanged (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return;
  }
  uispinbox->changed = false;
}

void
uiSpinboxAlignRight (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return;
  }
  gtk_entry_set_alignment (GTK_ENTRY (uispinbox->spinbox->widget), 1.0);
}

void
uiSpinboxAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  snprintf (tbuff, sizeof (tbuff), "spinbutton.%s", classnm);
  uiAddColorClass (tbuff, color);
}

uiwcont_t *
uiSpinboxGetWidgetContainer (uispinbox_t *uispinbox)
{
  if (uispinbox == NULL) {
    return NULL;
  }
  return uispinbox->spinbox;
}


/* internal routines */

/* gtk spinboxes are a bit bizarre */
static gint
uiSpinboxTextInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uispinbox_t   *uispinbox = udata;
  GtkAdjustment *adjustment;
  gdouble       value;

  /* text spinboxes do not allow text entry, so the value from the */
  /* adjustment is correct */

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (sb));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  if (uispinbox != NULL) {
    uispinbox->changed = true;
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
  uispinbox_t   *uispinbox = udata;
  GtkAdjustment *adjustment;
  gdouble       value;
  const char    *newtext;
  long          newvalue = -1;

  if (uispinbox->processing) {
    return UICB_NOT_CONVERTED;
  }
  uispinbox->processing = true;

  newtext = gtk_entry_get_text (GTK_ENTRY (uispinbox->spinbox->widget));
  if (uispinbox->convcb != NULL) {
    newvalue = callbackHandlerStr (uispinbox->convcb, newtext);
  } else {
    newvalue = tmutilStrToMS (newtext);
  }

  if (newvalue < 0) {
    uispinbox->processing = false;
    return UICB_NOT_CONVERTED;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uispinbox->spinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;
  if (newvalue != -1) {
    *newval = (double) newvalue;
  }
  uispinbox->changed = true;
  uispinbox->processing = false;
  return UICB_CONVERTED;
}

static gboolean
uiSpinboxTextDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t   *uispinbox = udata;
  GtkAdjustment *adjustment;
  const char    *disp;
  double        value;
  char          tbuff [100];


  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uispinbox->spinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uispinbox->spinbox->widget), value);
  uispinbox->curridx = (int) value;
  disp = "";
  if (uispinbox->textGetProc != NULL) {
    disp = uispinbox->textGetProc (uispinbox->udata, uispinbox->curridx);
  } else if (uispinbox->list != NULL) {
    disp = uiSpinboxTextGetDisp (uispinbox->list, uispinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", uispinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (uispinbox->spinbox->widget), tbuff);
  uispinbox->processing = false;
  return UICB_DISPLAY_ON;
}

static gboolean
uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t   *uispinbox = udata;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uispinbox->spinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uispinbox->spinbox->widget), value);
  if (uispinbox->sbtype == SB_TIME_BASIC) {
    tmutilToMS ((ssize_t) value, tbuff, sizeof (tbuff));
  }
  if (uispinbox->sbtype == SB_TIME_PRECISE) {
    tmutilToMSD ((ssize_t) value, tbuff, sizeof (tbuff), 1);
  }
  gtk_entry_set_text (GTK_ENTRY (uispinbox->spinbox->widget), tbuff);
  uispinbox->processing = false;
  return UICB_DISPLAY_ON;
}

static char *
uiSpinboxTextGetDisp (slist_t *list, int idx)
{
  return nlistGetDataByIdx (list, idx);
}

static bool
uiSpinboxTextKeyCallback (void *udata)
{
  uispinbox_t   *uispinbox = udata;
  bool          rc;

  rc = uiKeyIsMovementKey (uispinbox->uikey);
  if (rc) {
    return UICB_CONT;
  }
  rc = uiKeyIsNavKey (uispinbox->uikey);
  if (rc) {
    return UICB_CONT;
  }
  rc = uiKeyIsMaskedKey (uispinbox->uikey);
  if (rc) {
    return UICB_CONT;
  }

  return UICB_STOP;
}

static void
uiSpinboxValueChangedHandler (GtkSpinButton *sb, gpointer udata)
{
  callback_t  *uicb = udata;

  callbackHandler (uicb);
}

static gboolean
uiSpinboxDoubleDefaultDisplay (GtkSpinButton *sb, gpointer udata)
{
  uispinbox_t   *uispinbox = udata;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  if (uispinbox == NULL) {
    return UICB_DISPLAY_OFF;
  }
  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  if (uispinbox->spinbox == NULL) {
    return UICB_DISPLAY_OFF;
  }
  if (uispinbox->spinbox->widget == NULL) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uispinbox->spinbox->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uispinbox->spinbox->widget), value);
  if (value < 0) {
    /* CONTEXT: user interface: default setting display */
    gtk_entry_set_text (GTK_ENTRY (uispinbox->spinbox->widget), _("Default"));
  }
  uispinbox->processing = false;
  return UICB_DISPLAY_ON;
}

