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

#define UICB_STOP true
#define UICB_CONT false
#define UICB_DISPLAYED true
#define UICB_NO_DISP false
#define UICB_NO_CONV false
#define UICB_CONVERTED true

typedef struct {
#if BDJ4_USE_GTK
  union {
    GtkWidget         *widget;
    GtkSizeGroup      *sg;
    GdkPixbuf         *pixbuf;
    GtkTreeSelection  *sel;
    GtkTextBuffer     *buffer;
  };
#endif
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

typedef char * (*uispinboxdisp_t)(void *, int);

typedef struct {
  char          *title;
  GtkWidget     *parentwin;
  GtkWidget     *button;
  GtkWidget     *window;
  GtkWidget     *tree;
  GtkTreeSelection  *sel;
  slist_t       *strIndexMap;
  nlist_t       *keylist;
  gulong        closeHandlerId;
  char          *strSelection;
  bool          open : 1;
  bool          iscombobox : 1;
} uidropdown_t;

typedef struct {
  UIWidget      scw;
  UIWidget      textbox;
  UIWidget      buffer;
} uitextbox_t;

typedef struct uientry uientry_t;
typedef int (*uiutilsentryval_t)(uientry_t *entry, void *udata);

enum {
  UIENTRY_RESET,
  UIENTRY_ERROR,
  UIENTRY_OK,
};

typedef struct {
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

typedef struct uiswitch uiswitch_t;

extern int uiBaseMarginSz;

/* uiutils.c */
/* generic ui helper routines */
void  uiutilsCreateDanceList (uidropdown_t *dancesel, char *selectLabel);
uiutilsnbtabid_t * uiutilsNotebookIDInit (void);
void uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid);
void uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id);
int  uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx);
int  uiutilsNotebookIDGetPage (uiutilsnbtabid_t *nbtabid, int id);
void uiutilsNotebookIDStartIterator (uiutilsnbtabid_t *nbtabid, int *iteridx);
int  uiutilsNotebookIDIterate (uiutilsnbtabid_t *nbtabid, int *iteridx);
void uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source);
void uiutilsUICallbackInit (UICallback *uicb, UICallbackFunc cb, void *udata);
void uiutilsUICallbackDoubleInit (UICallback *uicb, UIDoubleCallbackFunc cb, void *udata);
void uiutilsUICallbackIntIntInit (UICallback *uicb, UIIntIntCallbackFunc cb, void *udata);
void uiutilsUICallbackLongInit (UICallback *uicb, UILongCallbackFunc cb, void *udata);
void uiutilsUICallbackLongIntInit (UICallback *uicb, UILongIntCallbackFunc cb, void *udata);
void uiutilsUICallbackStrInit (UICallback *uicb, UIStrCallbackFunc cb, void *udata);

#endif /* INC_UIUTILS_H */
