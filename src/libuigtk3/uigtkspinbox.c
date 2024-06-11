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

#include "ui/uievents.h"
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
  uiwcont_t       *uievent;
  bool            processing : 1;
  bool            changed : 1;
} uispinbox_t;

static uiwcont_t * uiSpinboxInit (void);
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

  uiEventFree (uispinbox->uievent);
  uiwcontBaseFree (uispinbox->uievent);

  mdfree (uispinbox);
}


uiwcont_t *
uiSpinboxTextCreate (void *udata)
{
  GtkWidget   *widget;
  uiwcont_t   *uiwidget;
  uispinbox_t *uispinbox;

  uiwidget = uiSpinboxInit ();
  uispinbox = uiwidget->uiint.uispinbox;

  widget = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 1.0, 1.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  uispinbox->udata = udata;

  uiwidget->wtype = WCONT_T_SPINBOX_TEXT;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;
  uiEventSetKeyCallback (uispinbox->uievent, uiwidget, uispinbox->presscb);

  uiWidgetSetClass (uiwidget, SPINBOX_READONLY_CLASS);

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTextDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTextInput), uiwidget);

  return uiwidget;
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
  gtk_entry_set_width_chars (GTK_ENTRY (uiwidget->uidata.widget), uispinbox->maxWidth + 2);
  gtk_entry_set_max_width_chars (GTK_ENTRY (uiwidget->uidata.widget), uispinbox->maxWidth + 2);
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
uiSpinboxTimeCreate (int sbtype, void *udata, callback_t *convcb)
{
  double      inca = 5000.0;
  double      incb = 60000.0;
  GtkWidget   *widget;
  uispinbox_t *uispinbox;
  uiwcont_t   *uiwidget;

  uiwidget = uiSpinboxInit ();
  uispinbox = uiwidget->uiint.uispinbox;

  uispinbox->sbtype = sbtype;

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
  /* 7200000 = 120 minutes */
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (widget), 0.0, 7200000.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);
  uispinbox->udata = udata;

  uiwidget->wtype = WCONT_T_SPINBOX_TIME;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxTimeDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxTimeInput), uiwidget);

  return uiwidget;
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

  spinbox = gtk_spin_button_new (NULL, 0.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 1.0, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);

  uiwidget->uidata.widget = spinbox;
  uiwidget->uidata.packwidget = spinbox;

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

  spinbox = gtk_spin_button_new (NULL, 0.0, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbox), TRUE);
  gtk_entry_set_alignment (GTK_ENTRY (spinbox), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (spinbox), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinbox), FALSE);
  gtk_widget_set_margin_top (spinbox, uiBaseMarginSz);
  gtk_widget_set_margin_start (spinbox, uiBaseMarginSz);

  uiwidget->uidata.widget = spinbox;
  uiwidget->uidata.packwidget = spinbox;

  g_signal_connect (spinbox, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);

  return uiwidget;
}

uiwcont_t *
uiSpinboxDoubleDefaultCreate (void)
{
  GtkWidget   *widget;
  uiwcont_t   *uiwidget;

  uiwidget = uiSpinboxInit ();

  widget = gtk_spin_button_new (NULL, -0.1, 1);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_entry_set_alignment (GTK_ENTRY (widget), 1.0);
  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (widget), 0.1, 5.0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), FALSE);
  gtk_widget_set_margin_top (widget, uiBaseMarginSz);
  gtk_widget_set_margin_start (widget, uiBaseMarginSz);

  uiwidget->wtype = WCONT_T_SPINBOX_DOUBLE_DFLT;
  uiwidget->uidata.widget = widget;
  uiwidget->uidata.packwidget = widget;

  g_signal_connect (widget, "output",
      G_CALLBACK (uiSpinboxDoubleDefaultDisplay), uiwidget);
  g_signal_connect (widget, "input",
      G_CALLBACK (uiSpinboxDoubleInput), NULL);

  return uiwidget;
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

  g_signal_connect (uiwidget->uidata.widget, "value-changed",
      G_CALLBACK (uiSpinboxValueChangedHandler), uicb);
}

void
uiSpinboxSetRange (uiwcont_t *uiwidget, double min, double max)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-range")) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uiwidget->uidata.widget), min, max);
}

void
uiSpinboxSetIncrement (uiwcont_t *uiwidget, double incr, double pageincr)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-incr")) {
    return;
  }

  gtk_spin_button_set_increments (GTK_SPIN_BUTTON (uiwidget->uidata.widget),
      incr, pageincr);
}


void
uiSpinboxWrap (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set-wrap")) {
    return;
  }

  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (uiwidget->uidata.widget), TRUE);
}

void
uiSpinboxSet (uiwcont_t *uiwidget, double min, double max)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-set")) {
    return;
  }

  gtk_spin_button_set_range (GTK_SPIN_BUTTON (uiwidget->uidata.widget), min, max);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->uidata.widget), min);
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
      GTK_SPIN_BUTTON (uiwidget->uidata.widget));
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
      GTK_SPIN_BUTTON (uiwidget->uidata.widget));
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
  if (uispinbox == NULL) {
    return false;
  }

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
  if (uispinbox != NULL) {
    uispinbox->changed = false;
  }
}

void
uiSpinboxAlignRight (uiwcont_t *uiwidget)
{
  if (! uiwcontValid (uiwidget, WCONT_T_SPINBOX, "spinbox-align-right")) {
    return;
  }

  gtk_entry_set_alignment (GTK_ENTRY (uiwidget->uidata.widget), 1.0);
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

static uiwcont_t *
uiSpinboxInit (void)
{
  uiwcont_t   *uiwidget;
  uispinbox_t *uispinbox;

  uiwidget = uiwcontAlloc ();
  uiwidget->wbasetype = WCONT_T_SPINBOX;
  uiwidget->wtype = WCONT_T_SPINBOX;

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
  uispinbox->uievent = uiEventAlloc ();
  uispinbox->presscb = callbackInit (&uiSpinboxTextKeyCallback, uiwidget, NULL);

  uiwidget->uiint.uispinbox = uispinbox;

  return uiwidget;
}


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

  newtext = gtk_entry_get_text (GTK_ENTRY (uiwidget->uidata.widget));
  if (uispinbox->convcb != NULL) {
    newvalue = callbackHandlerStr (uispinbox->convcb, newtext);
  } else {
    newvalue = tmutilStrToMS (newtext);
  }

  if (newvalue < 0) {
    uispinbox->processing = false;
    return UICB_NOT_CONVERTED;
  }

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->uidata.widget));
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
      GTK_SPIN_BUTTON (uiwidget->uidata.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->uidata.widget), value);
  uispinbox->curridx = (int) value;
  disp = "";
  if (uispinbox->textGetProc != NULL) {
    disp = uispinbox->textGetProc (uispinbox->udata, uispinbox->curridx);
  } else if (uispinbox->list != NULL) {
    disp = uiSpinboxTextGetDisp (uispinbox->list, uispinbox->curridx);
  }
  snprintf (tbuff, sizeof (tbuff), "%-*s", uispinbox->maxWidth, disp);
  gtk_entry_set_text (GTK_ENTRY (uiwidget->uidata.widget), tbuff);
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
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->uidata.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->uidata.widget), value);
  if (uispinbox->sbtype == SB_TIME_BASIC) {
    tmutilToMS ((ssize_t) value, tbuff, sizeof (tbuff));
  }
  if (uispinbox->sbtype == SB_TIME_PRECISE) {
    tmutilToMSD ((ssize_t) value, tbuff, sizeof (tbuff), 1);
  }
  gtk_entry_set_text (GTK_ENTRY (uiwidget->uidata.widget), tbuff);
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

  rc = uiEventIsMovementKey (uispinbox->uievent);
  if (rc) {
    return UICB_CONT;
  }
  rc = uiEventIsNavKey (uispinbox->uievent);
  if (rc) {
    return UICB_CONT;
  }

  rc = uiEventIsMaskedKey (uispinbox->uievent);
  if (rc) {
    /* masked keys are allowed, but not paste or cut */
    /* this allows the pass-through of keys like control-s */
    if (uiEventIsPasteCutKey (uispinbox->uievent)) {
      return UICB_STOP;
    }

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
  if (uiwidget->uidata.widget == NULL) {
    return UICB_DISPLAY_OFF;
  }
  uispinbox->processing = true;

  *tbuff = '\0';
  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (uiwidget->uidata.widget));
  value = gtk_adjustment_get_value (adjustment);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (uiwidget->uidata.widget), value);
  if (value < 0) {
    /* CONTEXT: user interface: default setting display */
    gtk_entry_set_text (GTK_ENTRY (uiwidget->uidata.widget), _("Default"));
  }
  uispinbox->processing = false;
  return UICB_DISPLAY_ON;
}

