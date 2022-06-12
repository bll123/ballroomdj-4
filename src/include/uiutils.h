#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include <stdbool.h>

#if BDJ4_USE_GTK
# include <gtk/gtk.h>
#endif

#include "nlist.h"
#include "slist.h"
#include "tmutil.h"

typedef bool  (*UICallbackFunc)(void *udata);
typedef bool  (*UIDoubleCallbackFunc)(void *udata, double value);
typedef bool  (*UIIntIntCallbackFunc)(void *udata, int a, int b);
typedef bool  (*UILongCallbackFunc)(void *udata, long value);
typedef bool  (*UILongIntCallbackFunc)(void *udata, long lval, int ival);
typedef long  (*UIStrCallbackFunc)(void *udata, const char *txt);

typedef struct {
  union {
    UICallbackFunc        cb;
    UIDoubleCallbackFunc  doublecb;
    UIIntIntCallbackFunc  intintcb;
    UILongCallbackFunc    longcb;
    UILongIntCallbackFunc longintcb;
    UIStrCallbackFunc     strcb;
  };
  void            *udata;
} UICallback;

/* these are defined based on the gtk values */
/* would change for a different gui package */
#define UICB_STOP true
#define UICB_CONT false
#define UICB_DISPLAYED true
#define UICB_NO_DISP false
#define UICB_NO_CONV false
#define UICB_CONVERTED true

typedef struct {
  union {
#if BDJ4_USE_GTK
    GtkWidget         *widget;
    GtkSizeGroup      *sg;
    GdkPixbuf         *pixbuf;
    GtkTreeSelection  *sel;
    GtkTextBuffer     *buffer;
#endif
  };
} UIWidget;

enum {
  UIUTILS_BASE_MARGIN_SZ = 2,
  UIUTILS_MENU_MAX = 5,
};

typedef struct {
  int             menucount;
  UIWidget        menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

enum {
  UI_TAB_MUSICQ,
  UI_TAB_SONGSEL,
  UI_TAB_SONGEDIT,
  UI_TAB_AUDIOID,
};

typedef struct {
  int     tabcount;
  int     *tabids;
} uiutilsnbtabid_t;

/* dialogs */
typedef struct uiselect uiselect_t;

// ### todo move this into uigtkdialog.c
typedef struct uiselect {
  char        *label;
  UIWidget    *window;
  const char  *startpath;
  const char  *mimefiltername;
  const char  *mimetype;
} uiselect_t;

/* entry */
typedef struct uientry uientry_t;
typedef int (*uiutilsentryval_t)(uientry_t *entry, void *udata);

/* spinbox */
enum {
  SB_TEXT,
  SB_TIME_BASIC,
  SB_TIME_PRECISE,
};

typedef char * (*uispinboxdisp_t)(void *, int);

/* dropdown */
typedef struct uidropdown uidropdown_t;

typedef struct {
  UIWidget      scw;
  UIWidget      textbox;
  UIWidget      buffer;
} uitextbox_t;

extern int uiBaseMarginSz;

/* uiutils.c */
/* generic ui helper routines */
uiutilsnbtabid_t * uiutilsNotebookIDInit (void);
void uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid);
void uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id);
int  uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx);
int  uiutilsNotebookIDGetPage (uiutilsnbtabid_t *nbtabid, int id);
void uiutilsNotebookIDStartIterator (uiutilsnbtabid_t *nbtabid, int *iteridx);
int  uiutilsNotebookIDIterate (uiutilsnbtabid_t *nbtabid, int *iteridx);
bool uiutilsUIWidgetSet (UIWidget *uiwidget);
void uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source);
void uiutilsUICallbackInit (UICallback *uicb, UICallbackFunc cb, void *udata);
void uiutilsUICallbackDoubleInit (UICallback *uicb, UIDoubleCallbackFunc cb, void *udata);
void uiutilsUICallbackIntIntInit (UICallback *uicb, UIIntIntCallbackFunc cb, void *udata);
void uiutilsUICallbackLongInit (UICallback *uicb, UILongCallbackFunc cb, void *udata);
void uiutilsUICallbackLongIntInit (UICallback *uicb, UILongIntCallbackFunc cb, void *udata);
void uiutilsUICallbackStrInit (UICallback *uicb, UIStrCallbackFunc cb, void *udata);

#endif /* INC_UIUTILS_H */
