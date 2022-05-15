#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include <stdbool.h>

#ifdef UI_USE_GTK3
# include <gtk/gtk.h>
#endif

#include "nlist.h"
#include "slist.h"
#include "tmutil.h"

typedef bool (*UICallbackFunc)(void *udata);

typedef struct {
  UICallbackFunc cb;
  void            *udata;
} UICallback;

#define UICB_STOP true
#define UICB_CONT false

typedef struct {
#ifdef UI_USE_GTK3
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
  GtkWidget       *menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

enum {
  UI_TAB_MUSICQ,
  UI_TAB_SONGSEL,
  UI_TAB_SONGEDIT,
  UI_TAB_AUDIOID,
};

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
  UIUTILS_DROPDOWN_COL_SB_PAD,
  UIUTILS_DROPDOWN_COL_MAX,
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
  int           maxwidth;
  bool          open : 1;
  bool          iscombobox : 1;
} uidropdown_t;

typedef struct {
  UIWidget      scw;
  UIWidget      textbox;
  UIWidget      buffer;
} uitextbox_t;

typedef bool (*uiutilsentryval_t)(void *entry, void *udata);

typedef struct {
  GtkEntryBuffer  *buffer;
  UIWidget        uientry;
  int             entrySize;
  int             maxSize;
  uiutilsentryval_t validateFunc;
  mstime_t        validateTimer;
  void            *udata;
} uientry_t;

typedef struct {
  GtkWidget             *spinbox;
  int                   curridx;
  uispinboxdisp_t  textGetProc;
  void                  *udata;
  int                   maxWidth;
  slist_t               *list;
  nlist_t               *keylist;
  nlist_t               *idxlist;
  bool                  indisp : 1;
  bool                  changed : 1;
} uispinbox_t;

typedef struct {
  char        *label;
  GtkWidget   *window;
  const char  *startpath;
  const char  *mimefiltername;
  const char  *mimetype;
} uiselect_t;

extern int uiBaseMarginSz;

/* uiutils.c */
/* generic ui helper routines */
void  uiutilsCreateDanceList (uidropdown_t *dancesel, char *selectLabel);
uiutilsnbtabid_t * uiutilsNotebookIDInit (void);
void uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid);
void uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id);
int uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx);
void uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source);
void uiutilsUICallbackInit (UICallback *uicb, UICallbackFunc cb, void *udata);

#endif /* INC_UIUTILS_H */
