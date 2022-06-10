#ifndef INC_UI_H
#define INC_UI_H

#include <stdbool.h>

#if BDJ4_USE_GTK
# include <gtk/gtk.h>
#endif

#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "uiutils.h"

/* uigeneral.c */
/* general routines that are called by the ui specific code */
void uiutilsUIWidgetInit (UIWidget *uiwidget);
bool uiutilsCallbackHandler (UICallback *uicb);
bool uiutilsCallbackLongHandler (UICallback *uicb, long value);
bool uiutilsCallbackLongIntHandler (UICallback *uicb, long lval, int ival);
bool uiutilsCallbackStrHandler (UICallback *uicb, const char *str);

/* uigtkdialog.c */

enum {
  RESPONSE_NONE,
  RESPONSE_DELETE_WIN,
  RESPONSE_CLOSE,
  RESPONSE_APPLY,
  RESPONSE_RESET,
};

typedef struct {
  char        *label;
  UIWidget    *window;
  const char  *startpath;
  const char  *mimefiltername;
  const char  *mimetype;
} uiselect_t;

char  *uiSelectDirDialog (uiselect_t *selectdata);
char  *uiSelectFileDialog (uiselect_t *selectdata);
void uiCreateDialog (UIWidget *uiwidget, UIWidget *window,
    UICallback *uicb, const char *title, ...);
void  uiDialogPackInDialog (UIWidget *uidialog, UIWidget *boxp);
void  uiDialogDestroy (UIWidget *uidialog);

/* uigtkmenu.c */
void uiCreateMenubar (UIWidget *uiwidget);
void uiCreateSubMenu (UIWidget *uimenuitem, UIWidget *uimenu);
void uiMenuCreateItem (UIWidget *uimenu, UIWidget *uimenuitem, const char *txt, UICallback *uicb);
void uiMenuCreateCheckbox (UIWidget *uimenu, UIWidget *uimenuitem,
    const char *txt, int active, UICallback *uicb);
void uiMenuInit (uimenu_t *menu);
void uiMenuAddMainItem (UIWidget *uimenubar, UIWidget *uimenuitem,
    uimenu_t *menu, const char *txt);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

/* uigtklabel.c */
void  uiCreateLabel (UIWidget *uiwidget, const char *label);
void  uiCreateColonLabel (UIWidget *uiwidget, const char *label);
void  uiLabelSetColor (UIWidget *uilabel, const char *color);
void  uiLabelDarkenColor (UIWidget *uilabel, const char *color);
void  uiLabelSetFont (UIWidget *uilabel, const char *font);
void  uiLabelSetText (UIWidget *uilabel, const char *text);
void  uiLabelEllipsizeOn (UIWidget *uiwidget);
void  uiLabelSetMaxWidth (UIWidget *uiwidget, int width);
void  uiLabelAlignEnd (UIWidget *uiwidget);

/* uigtkbutton.c */
void uiCreateButton (UIWidget *uiwidget, UICallback *uicb, char *title, char *imagenm);
void uiButtonSetPressCallback (UIWidget *uiwidget, UICallback *uicb);
void uiButtonSetReleaseCallback (UIWidget *uiwidget, UICallback *uicb);
void uiButtonSetImage (UIWidget *uiwidget, const char *imagenm, const char *tooltip);
void uiButtonSetImageIcon (UIWidget *uiwidget, const char *nm);
void uiButtonAlignLeft (UIWidget *widget);

/* uigtkentry.c */
enum {
  UIENTRY_IMMEDIATE,
  UIENTRY_DELAYED,
};

uientry_t *uiEntryInit (int entrySize, int maxSize);
void uiEntryFree (uientry_t *entry);
void uiEntryCreate (uientry_t *entry);
void uiEntrySetIcon (uientry_t *entry, const char *name);
void uiEntryClearIcon (uientry_t *entry);
UIWidget * uiEntryGetUIWidget (uientry_t *entry);
void uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry);
const char * uiEntryGetValue (uientry_t *entry);
void uiEntrySetValue (uientry_t *entry, const char *value);
void uiEntrySetColor (uientry_t *entry, const char *color);
void uiEntrySetValidate (uientry_t *entry,
    uiutilsentryval_t valfunc, void *udata, int valdelay);
int uiEntryValidate (uientry_t *entry, bool forceflag);
int uiEntryValidateDir (uientry_t *edata, void *udata);
int uiEntryValidateFile (uientry_t *edata, void *udata);
void uiEntryDisable (uientry_t *entry);
void uiEntryEnable (uientry_t *entry);

/* uigtkspinbox.c */
uispinbox_t *uiSpinboxTextInit (void);
void  uiSpinboxTextFree (uispinbox_t *spinbox);
void  uiSpinboxTextCreate (uispinbox_t *spinbox, void *udata);
void  uiSpinboxTextSet (uispinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist, uispinboxdisp_t textGetProc);
int   uiSpinboxTextGetIdx (uispinbox_t *spinbox);
int   uiSpinboxTextGetValue (uispinbox_t *spinbox);
void  uiSpinboxTextSetValue (uispinbox_t *spinbox, int ivalue);
void  uiSpinboxTextDisable (uispinbox_t *spinbox);
void  uiSpinboxTextEnable (uispinbox_t *spinbox);

void  uiSpinboxIntCreate (UIWidget *uiwidget);
void  uiSpinboxDoubleCreate (UIWidget *uiwidget);

uispinbox_t *uiSpinboxTimeInit (void);
void  uiSpinboxTimeFree (uispinbox_t *spinbox);
void  uiSpinboxTimeCreate (uispinbox_t *spinbox, void *udata, UICallback *convcb);
ssize_t uiSpinboxTimeGetValue (uispinbox_t *spinbox);
void  uiSpinboxTimeSetValue (uispinbox_t *spinbox, ssize_t value);

void  uiSpinboxSetRange (uispinbox_t *spinbox, long min, long max);
void  uiSpinboxWrap (uispinbox_t *spinbox);
void  uiSpinboxSet (UIWidget *uispinbox, double min, double max);
double uiSpinboxGetValue (UIWidget *uispinbox);
void  uiSpinboxSetValue (UIWidget *uispinbox, double ivalue);
bool  uiSpinboxIsChanged (uispinbox_t *spinbox);
void  uiSpinboxResetChanged (uispinbox_t *spinbox);
void  uiSpinboxAlignRight (uispinbox_t *spinbox);
void  uiSpinboxSetColor (uispinbox_t *spinbox, const char *color);
UIWidget * uiSpinboxGetUIWidget (uispinbox_t *spinbox);
void uiSpinboxTextSetValueChangedCallback (uispinbox_t *spinbox, UICallback *uicb);
void uiSpinboxTimeSetValueChangedCallback (uispinbox_t *spinbox, UICallback *uicb);
void uiSpinboxSetValueChangedCallback (UIWidget *uiwidget, UICallback *uicb);

/* uigtkdropdown.c */

uidropdown_t *uiDropDownInit (void);
void uiDropDownFree (uidropdown_t *dropdown);
GtkWidget * uiDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata);
GtkWidget * uiComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata);
nlistidx_t uiDropDownSelectionGet (uidropdown_t *dropdown, GtkTreePath *path);
void uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx);
void uiDropDownSelectionSetStr (uidropdown_t *dropdown, const char *stridx);
void uiDropDownDisable (uidropdown_t *dropdown);
void uiDropDownEnable (uidropdown_t *dropdown);
char *uiDropDownGetString (uidropdown_t *dropdown);

/* uigtklink.c */
void uiCreateLink (UIWidget *uiwidget, char *label, char *uri);
void uiLinkSet (UIWidget *uilink, char *label, char *uri);
void uiLinkSetActivateCallback (UIWidget *uilink, UICallback *uicb);

/* uigtktextbox.c */
uitextbox_t  *uiTextBoxCreate (int height);
void  uiTextBoxFree (uitextbox_t *tb);
UIWidget * uiTextBoxGetScrolledWindow (uitextbox_t *tb);
char  *uiTextBoxGetValue (uitextbox_t *tb);
void  uiTextBoxSetReadonly (uitextbox_t *tb);
void  uiTextBoxScrollToEnd (uitextbox_t *tb);
void  uiTextBoxAppendStr (uitextbox_t *tb, const char *str);
void  uiTextBoxAppendBoldStr (uitextbox_t *tb, const char *str);
void  uiTextBoxSetValue (uitextbox_t *tb, const char *str);
void  uiTextBoxDarken (uitextbox_t *tb);
void  uiTextBoxHorizExpand (uitextbox_t *tb);
void  uiTextBoxVertExpand (uitextbox_t *tb);
void  uiTextBoxSetHeight (uitextbox_t *tb, int h);

/* uigtknotebook.c */
void  uiCreateNotebook (UIWidget *uiwidget);
void  uiNotebookTabPositionLeft (UIWidget *uiwidget);
void  uiNotebookAppendPage (UIWidget *uinotebook, UIWidget *uiwidget, UIWidget *uilabel);
void  uiNotebookSetActionWidget (UIWidget *uinotebook, UIWidget *uiwidget);
void  uiNotebookSetPage (UIWidget *uinotebook, int pagenum);
void  uiNotebookSetCallback (UIWidget *uinotebook, UICallback *uicb);
void  uiNotebookHideShowPage (UIWidget *uinotebook, int pagenum, bool show);

/* uigtkbox.c */
void uiCreateVertBox (UIWidget *uiwidget);
void uiCreateHorizBox (UIWidget *uiwidget);
void uiBoxPackInWindow (UIWidget *uiwindow, UIWidget *uibox);
void uiBoxPackStart (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackStartExpand (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackEnd (UIWidget *uibox, UIWidget *uiwidget);
/* these routines will be removed at a later date */
void uiBoxPackInWindowUW (UIWidget *uiwindow, GtkWidget *widget);
void uiBoxPackInWindowWU (GtkWidget *window, UIWidget *uibox);
void uiBoxPackStartUW (UIWidget *uibox, GtkWidget *widget);
void uiBoxPackStartExpandUW (UIWidget *uibox, GtkWidget *widget);
void uiBoxPackEndUW (UIWidget *uibox, GtkWidget *widget);
/* these routines will be removed at a later date */
void uiBoxPackStartWU (GtkWidget *box, UIWidget *uiwidget);
void uiBoxPackStartExpandWU (GtkWidget *box, UIWidget *uiwidget);
void uiBoxPackEndWU (GtkWidget *box, UIWidget *uiwidget);
/* these routines will be removed at a later date */
GtkWidget *uiCreateVertBoxWW (void);
GtkWidget *uiCreateHorizBoxWW (void);
void uiBoxPackInWindowWW (GtkWidget *window, GtkWidget *box);
void uiBoxPackStartWW (GtkWidget *box, GtkWidget *widget);
void uiBoxPackStartExpandWW (GtkWidget *box, GtkWidget *widget);
void uiBoxPackEndWW (GtkWidget *box, GtkWidget *widget);

/* uigtkpbar.c */
void uiCreateProgressBar (UIWidget *uiwidget, char *color);
void uiProgressBarSet (UIWidget *uipb, double val);

/* uigtktreeview.c */

enum {
  UITREE_TYPE_NUM,
  UITREE_TYPE_STRING,
};

GtkWidget * uiCreateTreeView (void);
GtkTreeViewColumn * uiAddDisplayColumns (GtkWidget *tree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType * uiTreeViewAddDisplayType (GType *types, int valtype, int col);
void  uiTreeViewSetDisplayColumn (GtkTreeModel *model, GtkTreeIter *iter,
    int col, long num, const char *str);
int   uiTreeViewGetSelection (GtkWidget *tree, GtkTreeModel **model, GtkTreeIter *iter);
void  uiTreeViewAllowMultiple (GtkWidget *tree);

/* uigtkwindow.c */
void uiCreateMainWindow (UIWidget *uiwidget, UICallback *uicb,
    const char *title, const char *imagenm);
void uiCloseWindow (UIWidget *uiwindow);
bool uiWindowIsMaximized (UIWidget *uiwindow);
void uiWindowIconify (UIWidget *uiwindow);
void uiWindowMaximize (UIWidget *uiwindow);
void uiWindowUnMaximize (UIWidget *uiwindow);
void uiWindowDisableDecorations (UIWidget *uiwindow);
void uiWindowEnableDecorations (UIWidget *uiwindow);
void uiWindowGetSize (UIWidget *uiwindow, int *x, int *y);
void uiWindowSetDefaultSize (UIWidget *uiwindow, int x, int y);
void uiWindowGetPosition (UIWidget *uiwindow, int *x, int *y);
void uiWindowMove (UIWidget *uiwindow, int x, int y);
void uiWindowNoFocusOnStartup (UIWidget *uiwindow);
void uiCreateScrolledWindow (UIWidget *uiwindow, int minheight);
void uiWindowSetDoubleClickCallback (UIWidget *uiwindow, UICallback *uicb);
void uiWindowSetWinStateCallback (UIWidget *uiwindow, UICallback *uicb);
void uiWindowNoDim (UIWidget *uiwidget);
void uiWindowSetMappedCallback (UIWidget *uiwidget, UICallback *uicb);
/* these routines will be removed at a later date */
GtkWidget * uiCreateScrolledWindowW (int minheight);

/* uigtkscale.c */
void    uiCreateScale (UIWidget *uiwidget, double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits);
void    uiScaleSetCallback (UIWidget *uiscale, UICallback *uicb);
double  uiScaleEnforceMax (UIWidget *uiscale, double value);
double  uiScaleGetValue (UIWidget *uiscale);
int     uiScaleGetDigits (UIWidget *uiscale);
void    uiScaleSetValue (UIWidget *uiscale, double value);
void    uiScaleSetRange (UIWidget *uiscale, double start, double end);

/* uigtkswitch.c */

uiswitch_t *uiCreateSwitch (int value);
void uiSwitchFree (uiswitch_t *uiswitch);
void uiSwitchSetValue (uiswitch_t *uiswitch, int value);
int uiSwitchGetValue (uiswitch_t *uiswitch);
UIWidget *uiSwitchGetUIWidget (uiswitch_t *uiswitch);
void uiSwitchSetCallback (uiswitch_t *uiswitch, UICallback *uicb);
void uiSwitchDisable (uiswitch_t *uiswitch);
void uiSwitchEnable (uiswitch_t *uiswitch);

/* uigtksizegrp.c */
void uiCreateSizeGroupHoriz (UIWidget *);
void uiSizeGroupAdd (UIWidget *uiw, UIWidget *uiwidget);

/* uigtkutils.c */
void  uiUIInitialize (void);
void  uiUIProcessEvents (void);
void  uiCleanup (void);
void  uiSetCss (GtkWidget *w, char *style);
void  uiSetUIFont (char *uifont);
void  uiInitUILog (void);
void  uiGetForegroundColor (UIWidget *uiwidget, char *buff, size_t sz);
/* will be removed at a later date */
void  uiGetForegroundColorW (GtkWidget *widget, char *buff, size_t sz);

/* uigtkwidget.c */
/* widget interface */
void  uiWidgetDisable (UIWidget *uiwidget);
void  uiWidgetEnable (UIWidget *uiwidget);
void  uiWidgetExpandHoriz (UIWidget *uiwidget);
void  uiWidgetExpandVert (UIWidget *uiwidget);
void  uiWidgetSetAllMargins (UIWidget *uiwidget, int margin);
void  uiWidgetSetMarginTop (UIWidget *uiwidget, int margin);
void  uiWidgetSetMarginStart (UIWidget *uiwidget, int margin);
void  uiWidgetSetMarginEnd (UIWidget *uiwidget, int margin);
void  uiWidgetAlignHorizFill (UIWidget *uiwidget);
void  uiWidgetAlignHorizStart (UIWidget *uiwidget);
void  uiWidgetAlignHorizEnd (UIWidget *uiwidget);
void  uiWidgetAlignVertFill (UIWidget *uiwidget);
void  uiWidgetAlignVertStart (UIWidget *uiwidget);
void  uiWidgetDisableFocus (UIWidget *uiwidget);
void  uiWidgetHide (UIWidget *uiwidget);
void  uiWidgetShow (UIWidget *uiwidget);
void  uiWidgetShowAll (UIWidget *uiwidget);
void  uiWidgetMakePersistent (UIWidget *uiuiwidget);
void  uiWidgetClearPersistent (UIWidget *uiuiwidget);
void  uiWidgetSetSizeRequest (UIWidget *uiuiwidget, int width, int height);
bool  uiWidgetIsValid (UIWidget *uiwidget);
/* these routines will be removed at a later date */
void  uiWidgetExpandHorizW (GtkWidget *widget);
void  uiWidgetExpandVertW (GtkWidget *widget);
void  uiWidgetSetAllMarginsW (GtkWidget *widget, int margin);
void  uiWidgetSetMarginStartW (GtkWidget *widget, int margin);
void  uiWidgetAlignHorizFillW (GtkWidget *widget);
void  uiWidgetAlignVertFillW (GtkWidget *widget);

/* uigtkimage.c */
void  uiImageFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFree (UIWidget *uiwidget);

/* uigtktoggle.c */
void uiCreateCheckButton (UIWidget *uiwidget, const char *txt, int value);
void uiCreateToggleButton (UIWidget *uiwidget, const char *txt, const char *imgname,
    const char *tooltiptxt, UIWidget *image, int value);
void uiToggleButtonSetCallback (UIWidget *uiwidget, UICallback *uicb);
void uiToggleButtonSetImage (UIWidget *uiwidget, UIWidget *image);
bool uiToggleButtonIsActive (UIWidget *uiwidget);
void uiToggleButtonSetState (UIWidget *uiwidget, int state);

/* uigtkimage.c */
void uiImageNew (UIWidget *uiwidget);
void uiImageFromFile (UIWidget *uiwidget, const char *fn);
void uiImageScaledFromFile (UIWidget *uiwidget, const char *fn, int scale);
void uiImageClear (UIWidget *uiwidget);
void uiImageGetPixbuf (UIWidget *uiwidget);
void uiImageSetFromPixbuf (UIWidget *uiwidget, UIWidget *uipixbuf);

/* uigtksep.c */
void uiCreateHorizSeparator (UIWidget *uiwidget);
void uiSeparatorSetColor (UIWidget *uiwidget, const char *color);

#endif /* INC_UI_H */
