/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

uiwcont_t *
uiSpinboxInit (void)
{
  uiwcont_t   *uiwidget;
  uispinbox_t *uispinbox;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX;
  uiwidget->widget = NULL;

  uispinbox = mdmalloc (sizeof (uispinbox_t));
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

  uiwidget->uiint.uispinbox = uispinbox;

  return uiwidget;
}


/* only frees the internals */
void
uiSpinboxFree (uiwcont_t *uiwidget)
{
  uispinbox_t   *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-free")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;

  callbackFree (uispinbox->presscb);
  nlistFree (uispinbox->idxlist);
  uiwcontFree (uispinbox->uikey);
  mdfree (uispinbox);
}


void
uiSpinboxTextCreate (uiwcont_t *uiwidget, void *udata)
{
  GtkWidget   *widget;
  uispinbox_t *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-text-create")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;

  widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  uispinbox->udata = udata;

  uiwidget->wtype = WCONT_T_SPINBOX_TEXT;
  uiwidget->widget = widget;
  uiKeySetKeyCallback (uispinbox->uikey, uiwidget, uispinbox->presscb);

  uiWidgetSetClass (uiwidget, SPINBOX_READONLY_CLASS);

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTextDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTextInput), uiwidget);
}

void
uiSpinboxTextSet (uiwcont_t *uiwidget, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist,
    uispinboxdisp_t textGetProc)
{
  uispinbox_t   *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TEXT, "spinbox-text-set")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;

  uispinbox->maxWidth = maxWidth;
  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->widget), uispinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (uiwidget->widget), uispinbox->maxWidth + 2);
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
  uiSpinboxSet (uiwidget, (double) min, (double) (count - 1));
}

int
uiSpinboxTextGetIdx (uiwcont_t *uiwidget)
{
  int val;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TEXT, "spinbox-text-get-idx")) {
    return -1;
  }

  val = (int) uiSpinboxGetValue (uiwidget);
  return val;
}

int
uiSpinboxTextGetValue (uiwcont_t *uiwidget)
{
  int         nval;
  uispinbox_t *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TEXT, "spinbox-text-get-val")) {
    return -1;
  }

  uispinbox = uiwidget->uiint.uispinbox;
  nval = (int) uiSpinboxGetValue (uiwidget);
  if (uispinbox->keylist != NULL) {
    nval = nlistGetNum (uispinbox->keylist, nval);
  }
  return nval;
}

void
uiSpinboxTextSetValue (uiwcont_t *uiwidget, int value)
{
  nlistidx_t    idx;
  uispinbox_t   *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TEXT, "spinbox-text-set-val")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;
  idx = value;
  if (uispinbox->idxlist != NULL) {
    idx = nlistGetNum (uispinbox->idxlist, value);
  }
  uiSpinboxSetValue (uiwidget, (double) idx);
}

void
uiSpinboxTextSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TEXT, "spinbox-text-val-chg-cb")) {
    return;
  }

  uiSpinboxSetValueChangedCallback (uiwidget, uicb);
}

uiwcont_t *
uiSpinboxTimeInit (int sbtype)
{
  uiwcont_t   *uiwidget;
  uispinbox_t *uispinbox;

  uiwidget = uiSpinboxInit ();
  uispinbox = uiwidget->uiint.uispinbox;
  uispinbox->sbtype = sbtype;
  return uiwidget;
}

void
uiSpinboxTimeCreate (uiwcont_t *uiwidget, void *udata, callback_t *convcb)
{
  double      inca = 5000.0;
  double      incb = 60000.0;
  GtkWidget   *widget;
  uispinbox_t *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-time-create")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;

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
  uispinbox->udata = udata;

  uiwidget->wtype = WCONT_T_SPINBOX_TIME;
  uiwidget->widget = widget;

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTimeDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTimeInput), uiwidget);

  return;
}

ssize_t
uiSpinboxTimeGetValue (uiwcont_t *uiwidget)
{
  ssize_t value;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TIME, "spinbox-time-get-val")) {
    return -1;
  }

  value = (ssize_t) uiSpinboxGetValue (uiwidget);
  return value;
}

void
uiSpinboxTimeSetValue (uiwcont_t *uiwidget, ssize_t value)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TIME, "spinbox-time-set-val")) {
    return;
  }

  uiSpinboxSetValue (uiwidget, (double) value);
}

void
uiSpinboxTimeSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX_TIME, "spinbox-time-val-chg-cb")) {
    return;
  }

  uiSpinboxSetValueChangedCallback (uiwidget, uicb);
}

uiwcont_t *
uiSpinboxIntCreate (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *spinbox;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX_NUM;
  uiwidget->widget = NULL;
  uiwidget->uiint.uispinbox = NULL;

  spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 1.0, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);

  uiwidget->widget = spinbox;

  g_signal_connect (spinbox, "input",
      G_CALLBACK (uiSpinboxNumInput), NULL);

  return uiwidget;
}

uiwcont_t *
uiSpinboxDoubleCreate (void)
{
  uiwcont_t   *uiwidget;
  GtkWidget   *spinbox;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX_DOUBLE;
  uiwidget->widget = NULL;
  uiwidget->uiint.uispinbox = NULL;

  spinbox = gtk_spin_button_new (NULL, 0.0, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);

  uiwidget->widget = spinbox;

  g_signal_connect (spinbox, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);

  return uiwidget;
}

void
uiSpinboxDoubleDefaultCreate (uiwcont_t *uiwidget)
{
  GtkWidget   *widget;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-d-dflt-create")) {
    return;
  }

  widget = gtk_spin_button_new (NULL, -0.1, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_entry_set_alignment (GTK_ENTRY (widget), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  uiwidget->wtype = WCONT_T_SPINBOX_DOUBLE_DFLT;
  uiwidget->widget = widget;

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxDoubleDefaultDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);
}

void
uiSpinboxSetState (uiwcont_t *uiwidget, int state)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-state")) {
    return;
  }

  uiWidgetSetState (uiwidget, state);
}

void
uiSpinboxSetValueChangedCallback (uiwcont_t *uiwidget, callback_t *uicb)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-val-chg-cb")) {
    return;
  }

  g_signal_connect (uiwidget->widget, "value-changed",
      G_CALLBACK (uiSpinboxValueChangedHandler), uicb);
}

void
uiSpinboxSetRange (uiwcont_t *uiwidget, double min, double max)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-range")) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uiwidget->widget), min, max);
}

void
uiSpinboxSetIncrement (uiwcont_t *uiwidget, double incr, double pageincr)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-incr")) {
    return;
  }

  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (uiwidget->widget),
      incr, pageincr);
}


void
uiSpinboxWrap (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-wrap")) {
    return;
  }

  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (uiwidget->widget), TRUE);
}

void
uiSpinboxSet (uiwcont_t *uiwidget, double min, double max)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set")) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uiwidget->widget), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->widget), min);
}

double
uiSpinboxGetValue (uiwcont_t *uiwidget)
{
  GtkAdjustment     *adjustment;
  gdouble           value;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-get-val")) {
    return -1;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uiwidget->widget));
  value = gtk_adjustment_get_value (adjustment);
  return value;
}

void
uiSpinboxSetValue (uiwcont_t *uiwidget, double value)
{
  GtkAdjustment     *adjustment;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-val")) {
    return;
  }

  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uiwidget->widget));
  gtk_adjustment_set_value (adjustment, value);
}


bool
uiSpinboxIsChanged (uiwcont_t *uiwidget)
{
  uispinbox_t *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-is-chg")) {
    return false;
  }

  uispinbox = uiwidget->uiint.uispinbox;
  return uispinbox->changed;
}

void
uiSpinboxResetChanged (uiwcont_t *uiwidget)
{
  uispinbox_t *uispinbox;

  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-reset-chg")) {
    return;
  }

  uispinbox = uiwidget->uiint.uispinbox;
  uispinbox->changed = false;
}

void
uiSpinboxAlignRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-align-right")) {
    return;
  }

  gtk_entry_set_alignment (GTK_ENTRY (uiwidget->widget), 1.0);
}

void
uiSpinboxAddClass (const char *classnm, const char *color)
{
  char    tbuff [100];

  if (classnm == NULL || color == NULL) {
    return;
  }

  snprintf (tbuff, sizeof (tbuff), "spinbutton.%s", classnm);
  uiAddColorClass (tbuff, color);
}

/* internal routines */

/* gtk spinboxes are a bit bizarre */
static gint
uiSpinboxTextInput (GtkSpinButton *sb, gdouble *newval, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  GtkAdjustment *adjustment;
  gdouble       value;

  /* text spinboxes do not allow text entry, so the value from the */
  /* adjustment is correct */

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (sb));
  value = gtk_adjustment_get_value (adjustment);
  *newval = value;

  uispinbox = uiwidget->uiint.uispinbox;
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
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  GtkAdjustment *adjustment;
  gdouble       value;
  const char    *newtext;
  long          newvalue = -1;


  uispinbox = uiwidget->uiint.uispinbox;

  if (uispinbox->processing) {
    return UICB_NOT_CONVERTED;
  }
  uispinbox->processing = true;

  newtext = gtk_entry_get_text (GTK_ENTRY (uiwidget->widget));
  if (uispinbox->convcb != NULL) {
    newvalue = callbackHandlerStr (uispinbox->convcb, newtext);
  } else {
    newvalue = tmutilStrToMS (newtext);
  }

  if (newvalue < 0) {
    uispinbox->processing = false;
    return UICB_NOT_CONVERTED;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->widget));
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
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  GtkAdjustment *adjustment;
  const char    *disp;
  double        value;
  char          tbuff [100];

  uispinbox = uiwidget->uiint.uispinbox;

  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (
      GTK_SPIN_BUTTON (uiwidget->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->widget), value);
  uispinbox->curridx = (int) value;
  disp = "";
  if (uispinbox->textGetProc != NULL) {
    disp = uispinbox->textGetProc (uispinbox->udata, uispinbox->curridx);
  } else if (uispinbox->list != NULL) {
    disp = uiSpinboxTextGetDisp (uispinbox->list, uispinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", uispinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (uiwidget->widget), tbuff);
  uispinbox->processing = false;

  return UICB_DISPLAY_ON;
}

static gboolean
uiSpinboxTimeDisplay (GtkSpinButton *sb, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  uispinbox = uiwidget->uiint.uispinbox;

  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->widget), value);
  if (uispinbox->sbtype == SB_TIME_BASIC) {
    tmutilToMS ((ssize_t) value, tbuff, sizeof (tbuff));
  }
  if (uispinbox->sbtype == SB_TIME_PRECISE) {
    tmutilToMSD ((ssize_t) value, tbuff, sizeof (tbuff), 1);
  }
  gtk_entry_set_text (GTK_ENTRY (uiwidget->widget), tbuff);
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
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  bool          rc;

  uispinbox = uiwidget->uiint.uispinbox;

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

  if (uicb == NULL) {
    return;
  }

  callbackHandler (uicb);
}

static gboolean
uiSpinboxDoubleDefaultDisplay (GtkSpinButton *sb, gpointer udata)
{
  uiwcont_t     *uiwidget = udata;
  uispinbox_t   *uispinbox;
  GtkAdjustment *adjustment;
  double        value;
  char          tbuff [100];

  uispinbox = uiwidget->uiint.uispinbox;

  if (uispinbox == NULL) {
    return UICB_DISPLAY_OFF;
  }
  if (uispinbox->processing) {
    return UICB_DISPLAY_OFF;
  }
  if (uiwidget == NULL) {
    return UICB_DISPLAY_OFF;
  }
  if (uiwidget->widget == NULL) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->widget), value);
  if (value < 0) {
    /* CONTEXT: user interface: default setting display */
    gtk_entry_set_text (GTK_ENTRY (uiwidget->widget), _("Default"));
  }
  uispinbox->processing = false;
  return UICB_DISPLAY_ON;
}

