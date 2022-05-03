#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tmutil.h"

enum {
  UIUTILS_BASE_MARGIN_SZ = 2,
  UI_MAIN_LOOP_TIMER  = 30,
  UIUTILS_MENU_MAX = 5,
};

typedef struct {
  int             menucount;
  GtkWidget       *menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uiutilsmenu_t;

enum {
  FILTER_DISP_GENRE,
  FILTER_DISP_DANCELEVEL,
  FILTER_DISP_STATUS,
  FILTER_DISP_FAVORITE,
  FILTER_DISP_STATUSPLAYABLE,
  FILTER_DISP_MAX,
};

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

extern datafilekey_t filterdisplaydfkeys [];
extern int uiutilsBaseMarginSz;

/* uiutils.c */
/* generic ui helper routines */
void  uiutilsCreateDanceList (uiutilsdropdown_t *dancesel, char *selectLabel);
uiutilsnbtabid_t * uiutilsNotebookIDInit (void);
void uiutilsNotebookIDFree (uiutilsnbtabid_t *nbtabid);
void uiutilsNotebookIDAdd (uiutilsnbtabid_t *nbtabid, int id);
int uiutilsNotebookIDGet (uiutilsnbtabid_t *nbtabid, int idx);

/* uigtkmenu.c */
GtkWidget * uiutilsCreateMenubar (void);
GtkWidget * uiutilsCreateSubMenu (GtkWidget *menuitem);
GtkWidget * uiutilsMenuCreateItem (GtkWidget *menu, const char *txt,
    void *activateAction, void *udata);
GtkWidget * uiutilsMenuCreateCheckbox (GtkWidget *menu, const char *txt,
    int active, void *toggleAction, void *udata);
void uiutilsMenuInit (uiutilsmenu_t *menu);
GtkWidget * uiutilsMenuAddMainItem (GtkWidget *menubar, uiutilsmenu_t *menu, const char *txt);
void uiutilsMenuDisplay (uiutilsmenu_t *menu);
void uiutilsMenuClear (uiutilsmenu_t *menu);

/* uigtklabel.c */
GtkWidget * uiutilsCreateLabel (const char *label);
GtkWidget * uiutilsCreateColonLabel (const char *label);
void        uiutilsLabelSetText (GtkWidget *label, const char *text);
void        uiutilsLabelEllipsizeOn (GtkWidget *widget);
void        uiutilsLabelSetMaxWidth (GtkWidget *widget, int width);

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
void  uiutilsTextBoxFree (uiutilstextbox_t *tb);
char  *uiutilsTextBoxGetValue (uiutilstextbox_t *tb);
void  uiutilsTextBoxSetReadonly (uiutilstextbox_t *tb);
void  uiutilsTextBoxScrollToEnd (uiutilstextbox_t *tb);
void  uiutilsTextBoxAppendStr (uiutilstextbox_t *tb, const char *str);
void  uiutilsTextBoxSetValue (uiutilstextbox_t *tb, const char *str);
void  uiutilsTextBoxDarken (uiutilstextbox_t *tb);
void  uiutilsTextBoxVertExpand (uiutilstextbox_t *tb);
void  uiutilsTextBoxSetHeight (uiutilstextbox_t *tb, int h);

/* uigtknotebook.c */
GtkWidget * uiutilsCreateNotebook (void);
void  uiutilsNotebookAppendPage (GtkWidget *notebook, GtkWidget *widget, GtkWidget *label);
void  uiutilsNotebookSetActionWidget (GtkWidget *notebook, GtkWidget *widget, GtkPackType pack);

/* uigtkbox.c */
GtkWidget *uiutilsCreateVertBox (void);
GtkWidget *uiutilsCreateHorizBox (void);
void uiutilsBoxPackInWindow (GtkWidget *window, GtkWidget *box);
void uiutilsBoxPackStart (GtkWidget *box, GtkWidget *widget);
void uiutilsBoxPackEnd (GtkWidget *box, GtkWidget *widget);

/* uigtkpbar.c */
GtkWidget * uiutilsCreateProgressBar (char *color);
void        uiutilsProgressBarSet (GtkWidget *pb, double val);

/* uigtktreeview.c */
GtkWidget * uiutilsCreateTreeView (void);
GType       * uiutilsAppendType (GType *types, int *ncol, int type);
GtkTreeViewColumn * uiutilsAddDisplayColumns (GtkWidget *tree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType *     uiutilsAddDisplayTypes (GType *types, slist_t *sellist, int *col);
void        uiutilsSetDisplayColumns (GtkListStore *store, GtkTreeIter *iter,
    slist_t *sellist, song_t *song, int col);

/* uigtkdialog.c */
char  *uiutilsSelectDirDialog (uiutilsselect_t *selectdata);
char  *uiutilsSelectFileDialog (uiutilsselect_t *selectdata);

/* uigtkwindow.c */
GtkWidget * uiutilsCreateMainWindow (char *title,
    char *imagenm, void *cb, void *udata);
void uiutilsCloseMainWindow (GtkWidget *window);
bool uiutilsWindowIsMaximized (GtkWidget *window);
void uiutilsWindowIconify (GtkWidget *window);
void uiutilsWindowMaximize (GtkWidget *window);
void uiutilsWindowUnMaximize (GtkWidget *window);
void uiutilsWindowDisableDecorations (GtkWidget *window);
void uiutilsWindowEnableDecorations (GtkWidget *window);
void uiutilsWindowGetSize (GtkWidget *window, int *x, int *y);
void uiutilsWindowSetDefaultSize (GtkWidget *window, int x, int y);
void uiutilsWindowGetPosition (GtkWidget *window, int *x, int *y);
void uiutilsWindowMove (GtkWidget *window, int x, int y);
void uiutilsWindowNoFocusOnStartup (GtkWidget *window);
GtkWidget * uiutilsCreateScrolledWindow (void);

/* uigtkscale.c */
GtkWidget * uiutilsCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue);
double    uiutilsScaleEnforceMax (GtkWidget *scale, double value);

/* uigtkswitch.c */
GtkWidget * uiutilsCreateSwitch (int value);
void uiutilsSwitchSetValue (GtkWidget *w, int value);

/* uigtkutils.c */
void  uiutilsUIInitialize (void);
void  uiutilsUIProcessEvents (void);
void  uiutilsCleanup (void);
void  uiutilsSetCss (GtkWidget *w, char *style);
void  uiutilsSetUIFont (char *uifont);
void  uiutilsInitUILog (void);
void  uiutilsGetForegroundColor (GtkWidget *widget, char *buff, size_t sz);
void  uiutilsWidgetDisable (GtkWidget *widget);
void  uiutilsWidgetEnable (GtkWidget *widget);
void  uiutilsWidgetExpandHoriz (GtkWidget *widget);
void  uiutilsWidgetExpandVert (GtkWidget *widget);
void  uiutilsWidgetAlignHorizFill (GtkWidget *widget);
void  uiutilsWidgetAlignHorizStart (GtkWidget *widget);
void  uiutilsWidgetAlignHorizEnd (GtkWidget *widget);
void  uiutilsWidgetAlignVertFill (GtkWidget *widget);
void  uiutilsWidgetAlignVertStart (GtkWidget *widget);
void  uiutilsWidgetDisableFocus (GtkWidget *widget);
void  uiutilsWidgetHide (GtkWidget *widget);
void  uiutilsWidgetShow (GtkWidget *widget);
void  uiutilsWidgetShowAll (GtkWidget *widget);
void  uiutilsWidgetSetAllMargins (GtkWidget *widget, int margin);

GtkWidget * uiutilsCreateCheckButton (const char *txt, int value);

#endif /* INC_UIUTILS_H */
