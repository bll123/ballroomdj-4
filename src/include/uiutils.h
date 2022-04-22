#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include <stdbool.h>

#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tmutil.h"

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
  UIUTILS_DROPDOWN_COL_SB_PAD,
  UIUTILS_DROPDOWN_COL_MAX,
};

typedef char * (*uiutilsspinboxdisp_t)(void *, int);

typedef struct {
  GtkWidget     *parentwin;
  GtkWidget     *button;
  GtkWidget     *window;
  GtkWidget     *tree;
  nlist_t       *numIndexMap;
  slist_t       *strIndexMap;
  gulong        closeHandlerId;
  char          *strSelection;
  bool          open : 1;
  bool          iscombobox : 1;
} uiutilsdropdown_t;

typedef struct {
  GtkWidget     *scw;
  GtkWidget     *textbox;
  GtkTextBuffer *buffer;
} uiutilstextbox_t;

typedef bool (*uiutilsentryval_t)(void *entry, void *udata);

typedef struct {
  GtkEntryBuffer  *buffer;
  GtkWidget       *entry;
  int             entrySize;
  int             maxSize;
  uiutilsentryval_t validateFunc;
  mstime_t        validateTimer;
  void            *udata;
} uiutilsentry_t;

typedef struct {
  GtkWidget             *spinbox;
  int                   curridx;
  uiutilsspinboxdisp_t  textGetProc;
  void                  *udata;
  int                   maxWidth;
  slist_t               *list;
  bool                  indisp : 1;
  bool                  changed : 1;
} uiutilsspinbox_t;

typedef struct {
  char        *label;
  GtkWidget   *window;
  const char  *startpath;
  const char  *mimefiltername;
  const char  *mimetype;
} uiutilsselect_t;

#define UI_MAIN_LOOP_TIMER 20

/* uiutils.c */
/* generic ui helper routines */
void  uiutilsCreateDanceList (uiutilsdropdown_t *dancesel, char *selectLabel);

/* uigtklabel.c */
GtkWidget * uiutilsCreateLabel (char *label);
GtkWidget * uiutilsCreateColonLabel (char *label);
void        uiutilsLabelSetText (GtkWidget *label, char *text);

/* uigtkbutton.c */
GtkWidget * uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata);

/* uigtkentry.c */
void uiutilsEntryInit (uiutilsentry_t *entry, int entrySize, int maxSize);
void uiutilsEntryFree (uiutilsentry_t *entry);
GtkWidget * uiutilsEntryCreate (uiutilsentry_t *entry);
GtkWidget * uiutilsEntryGetWidget (uiutilsentry_t *entry);
const char * uiutilsEntryGetValue (uiutilsentry_t *entry);
void uiutilsEntrySetValue (uiutilsentry_t *entry, char *value);
void uiutilsEntrySetValidate (uiutilsentry_t *entry,
    uiutilsentryval_t valfunc, void *udata);
bool uiutilsEntryValidate (uiutilsentry_t *entry);
bool uiutilsEntryValidateDir (void *edata, void *udata);
bool uiutilsEntryValidateFile (void *edata, void *udata);

/* uigtkspinbox.c */
void uiutilsSpinboxTextInit (uiutilsspinbox_t *spinbox);
void uiutilsSpinboxTextFree (uiutilsspinbox_t *spinbox);
GtkWidget * uiutilsSpinboxTextCreate (uiutilsspinbox_t *spinbox, void *udata);
void uiutilsSpinboxTextSet (uiutilsspinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, uiutilsspinboxdisp_t textGetProc);
int   uiutilsSpinboxTextGetValue (uiutilsspinbox_t *spinbox);
void  uiutilsSpinboxTextSetValue (uiutilsspinbox_t *spinbox, int ivalue);

GtkWidget * uiutilsSpinboxIntCreate (void);
GtkWidget * uiutilsSpinboxDoubleCreate (void);

void  uiutilsSpinboxSet (GtkWidget *spinbox, double min, double max);
double uiutilsSpinboxGetValue (GtkWidget *spinbox);
void  uiutilsSpinboxSetValue (GtkWidget *spinbox, double ivalue);
bool  uiutilsSpinboxIsChanged (uiutilsspinbox_t *spinbox);

void  uiutilsSpinboxTimeInit (uiutilsspinbox_t *spinbox);
void  uiutilsSpinboxTimeFree (uiutilsspinbox_t *spinbox);
GtkWidget * uiutilsSpinboxTimeCreate (uiutilsspinbox_t *spinbox, void *udata);
ssize_t uiutilsSpinboxTimeGetValue (uiutilsspinbox_t *spinbox);
void  uiutilsSpinboxTimeSetValue (uiutilsspinbox_t *spinbox, ssize_t value);

/* uigtkdropdown.c */
void uiutilsDropDownInit (uiutilsdropdown_t *dropdown);
void uiutilsDropDownFree (uiutilsdropdown_t *dropdown);
GtkWidget * uiutilsDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata);
GtkWidget * uiutilsComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uiutilsdropdown_t *dropdown, void *udata);
ssize_t uiutilsDropDownSelectionGet (uiutilsdropdown_t *dropdown,
    GtkTreePath *path);
void uiutilsDropDownSetList (uiutilsdropdown_t *dropdown, slist_t *list,
    char *selectLabel);
void uiutilsDropDownSetNumList (uiutilsdropdown_t *dropdown, slist_t *list,
    char *selectLabel);
void uiutilsDropDownSelectionSetNum (uiutilsdropdown_t *dropdown, nlistidx_t idx);
void uiutilsDropDownSelectionSetStr (uiutilsdropdown_t *dropdown, char *stridx);

/* uigtklink.c */
GtkWidget * uiutilsCreateLink (char *label, char *uri);
void        uiutilsLinkSet (GtkWidget *widget, char *label, char *uri);

/* uigtktextbox.c */
uiutilstextbox_t  *uiutilsTextBoxCreate (void);
char  *uiutilsTextBoxGetValue (uiutilstextbox_t *tb);
void  uiutilsTextBoxSetReadonly (uiutilstextbox_t *tb);
void  uiutilsTextBoxScrollToEnd (uiutilstextbox_t *tb);

/* uigtkutils.c */
void  uiutilsCleanup (void);
void  uiutilsSetCss (GtkWidget *w, char *style);
void  uiutilsSetUIFont (char *uifont);
void  uiutilsInitUILog (void);
int   uiutilsCreateApplication (int argc, char *argv [],
    char *tag, GtkApplication **app, void *activateFunc, void *udata);
void  uiutilsGetForegroundColor (GtkWidget *widget, char *buff, size_t sz);

GtkWidget * uiutilsCreateScrolledWindow (void);
GtkWidget * uiutilsCreateSwitch (int value);
GtkWidget * uiutilsCreateCheckButton (const char *txt, int value);
GtkWidget * uiutilsCreateNotebook (void);
GtkWidget * uiutilsCreateTreeView (void);

char  *uiutilsSelectDirDialog (uiutilsselect_t *selectdata);
char  *uiutilsSelectFileDialog (uiutilsselect_t *selectdata);

GType       * uiutilsAppendType (GType *types, int *ncol, int type);
GtkTreeViewColumn * uiutilsAddDisplayColumns (GtkWidget *tree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType *     uiutilsAddDisplayTypes (GType *types, slist_t *sellist, int *col);
void        uiutilsSetDisplayColumns (GtkListStore *store, GtkTreeIter *iter,
    slist_t *sellist, song_t *song, int col);

#endif /* INC_UIUTILS_H */
