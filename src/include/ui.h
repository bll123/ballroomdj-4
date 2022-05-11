#ifndef INC_UI_H
#define INC_UI_H

#include <stdbool.h>

#ifdef UI_USE_GTK3
# include <gtk/gtk.h>
#endif

#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "uiutils.h"

/* uigtkmenu.c */
GtkWidget * uiCreateMenubar (void);
GtkWidget * uiCreateSubMenu (GtkWidget *menuitem);
GtkWidget * uiMenuCreateItem (GtkWidget *menu, const char *txt,
    void *activateAction, void *udata);
GtkWidget * uiMenuCreateCheckbox (GtkWidget *menu, const char *txt,
    int active, void *toggleAction, void *udata);
void uiMenuInit (uimenu_t *menu);
GtkWidget * uiMenuAddMainItem (GtkWidget *menubar, uimenu_t *menu, const char *txt);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

/* uigtklabel.c */
GtkWidget * uiCreateLabel (const char *label);
GtkWidget * uiCreateColonLabel (const char *label);
void        uiLabelSetText (GtkWidget *label, const char *text);
void        uiLabelEllipsizeOn (GtkWidget *widget);
void        uiLabelSetMaxWidth (GtkWidget *widget, int width);

/* uigtkbutton.c */
GtkWidget * uiCreateButton (UIWidget *uiwidget, char *title,
    char *imagenm, void *cb, void *udata);
void uiButtonAlignLeft (GtkWidget *widget);

/* uigtkentry.c */
void uiEntryInit (uientry_t *entry, int entrySize, int maxSize);
void uiEntryFree (uientry_t *entry);
GtkWidget * uiEntryCreate (uientry_t *entry);
void uiEntryPeerBuffer (uientry_t *targetentry, uientry_t *sourceentry);
GtkWidget * uiEntryGetWidget (uientry_t *entry);
const char * uiEntryGetValue (uientry_t *entry);
void uiEntrySetValue (uientry_t *entry, const char *value);
void uiEntrySetValidate (uientry_t *entry,
    uiutilsentryval_t valfunc, void *udata);
bool uiEntryValidate (uientry_t *entry);
bool uiEntryValidateDir (void *edata, void *udata);
bool uiEntryValidateFile (void *edata, void *udata);

/* uigtkspinbox.c */
void uiSpinboxTextInit (uispinbox_t *spinbox);
void uiSpinboxTextFree (uispinbox_t *spinbox);
GtkWidget * uiSpinboxTextCreate (uispinbox_t *spinbox, void *udata);
void uiSpinboxTextSet (uispinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist, uispinboxdisp_t textGetProc);
int   uiSpinboxTextGetIdx (uispinbox_t *spinbox);
int   uiSpinboxTextGetValue (uispinbox_t *spinbox);
void  uiSpinboxTextSetValue (uispinbox_t *spinbox, int ivalue);

GtkWidget * uiSpinboxIntCreate (void);
GtkWidget * uiSpinboxDoubleCreate (void);

void  uiSpinboxSet (GtkWidget *spinbox, double min, double max);
double uiSpinboxGetValue (GtkWidget *spinbox);
void  uiSpinboxSetValue (GtkWidget *spinbox, double ivalue);
bool  uiSpinboxIsChanged (uispinbox_t *spinbox);

void  uiSpinboxTimeInit (uispinbox_t *spinbox);
void  uiSpinboxTimeFree (uispinbox_t *spinbox);
GtkWidget * uiSpinboxTimeCreate (uispinbox_t *spinbox, void *udata);
ssize_t uiSpinboxTimeGetValue (uispinbox_t *spinbox);
void  uiSpinboxTimeSetValue (uispinbox_t *spinbox, ssize_t value);

/* uigtkdropdown.c */
void uiDropDownInit (uidropdown_t *dropdown);
void uiDropDownFree (uidropdown_t *dropdown);
GtkWidget * uiDropDownCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata);
GtkWidget * uiComboboxCreate (GtkWidget *parentwin,
    char *title, void *processSelectionCallback,
    uidropdown_t *dropdown, void *udata);
ssize_t uiDropDownSelectionGet (uidropdown_t *dropdown,
    GtkTreePath *path);
void uiDropDownSetList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSetNumList (uidropdown_t *dropdown, slist_t *list,
    const char *selectLabel);
void uiDropDownSelectionSetNum (uidropdown_t *dropdown, nlistidx_t idx);
void uiDropDownSelectionSetStr (uidropdown_t *dropdown, char *stridx);

/* uigtklink.c */
GtkWidget * uiCreateLink (char *label, char *uri);
void        uiLinkSet (GtkWidget *widget, char *label, char *uri);

/* uigtktextbox.c */
uitextbox_t  *uiTextBoxCreate (int height);
void  uiTextBoxFree (uitextbox_t *tb);
char  *uiTextBoxGetValue (uitextbox_t *tb);
void  uiTextBoxSetReadonly (uitextbox_t *tb);
void  uiTextBoxScrollToEnd (uitextbox_t *tb);
void  uiTextBoxAppendStr (uitextbox_t *tb, const char *str);
void  uiTextBoxSetValue (uitextbox_t *tb, const char *str);
void  uiTextBoxDarken (uitextbox_t *tb);
void  uiTextBoxVertExpand (uitextbox_t *tb);
void  uiTextBoxSetHeight (uitextbox_t *tb, int h);

/* uigtknotebook.c */
GtkWidget * uiCreateNotebook (void);
void  uiNotebookAppendPage (GtkWidget *notebook, GtkWidget *widget, GtkWidget *label);
void  uiNotebookSetActionWidget (GtkWidget *notebook, GtkWidget *widget, GtkPackType pack);
void  uiNotebookSetPage (GtkWidget *notebook, int pagenum);

/* uigtkbox.c */
GtkWidget *uiCreateVertBox (void);
GtkWidget *uiCreateHorizBox (void);
void uiBoxPackInWindow (GtkWidget *window, GtkWidget *box);
void uiBoxPackStart (GtkWidget *box, GtkWidget *widget);
void uiBoxPackStartExpand (GtkWidget *box, GtkWidget *widget);
void uiBoxPackEnd (GtkWidget *box, GtkWidget *widget);

/* uigtkpbar.c */
GtkWidget * uiCreateProgressBar (char *color);
void        uiProgressBarSet (GtkWidget *pb, double val);

/* uigtktreeview.c */
GtkWidget * uiCreateTreeView (void);
GType * uiAppendType (GType *types, int *ncol, int type);
GtkTreeViewColumn * uiAddDisplayColumns (GtkWidget *tree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType * uiAddDisplayTypes (GType *types, slist_t *sellist, int *col);
void  uiSetDisplayColumns (GtkListStore *store, GtkTreeIter *iter,
    slist_t *sellist, song_t *song, int col);
int   uiTreeViewGetSelection (GtkWidget *tree, GtkTreeModel **model, GtkTreeIter *iter);
void  uiTreeViewAllowMultiple (GtkWidget *tree);


/* uigtkdialog.c */
char  *uiSelectDirDialog (uiselect_t *selectdata);
char  *uiSelectFileDialog (uiselect_t *selectdata);

/* uigtkwindow.c */
GtkWidget * uiCreateMainWindow (char *title,
    char *imagenm, void *cb, void *udata);
void uiCloseWindow (GtkWidget *window);
bool uiWindowIsMaximized (GtkWidget *window);
void uiWindowIconify (GtkWidget *window);
void uiWindowMaximize (GtkWidget *window);
void uiWindowUnMaximize (GtkWidget *window);
void uiWindowDisableDecorations (GtkWidget *window);
void uiWindowEnableDecorations (GtkWidget *window);
void uiWindowGetSize (GtkWidget *window, int *x, int *y);
void uiWindowSetDefaultSize (GtkWidget *window, int x, int y);
void uiWindowGetPosition (GtkWidget *window, int *x, int *y);
void uiWindowMove (GtkWidget *window, int x, int y);
void uiWindowNoFocusOnStartup (GtkWidget *window);
GtkWidget * uiCreateScrolledWindow (int minheight);

/* uigtkscale.c */
GtkWidget * uiCreateScale (double lower, double upper,
    double stepinc, double pageinc, double initvalue);
double    uiScaleEnforceMax (GtkWidget *scale, double value);

/* uigtkswitch.c */
GtkWidget * uiCreateSwitch (int value);
void uiSwitchSetValue (GtkWidget *w, int value);

/* uigtksizegrp.c */
void uiCreateSizeGroupHoriz (UIWidget *);
void uiSizeGroupAdd (UIWidget *uiw, GtkWidget *widget);

/* uigtkutils.c */
void  uiUIInitialize (void);
void  uiUIProcessEvents (void);
void  uiCleanup (void);
void  uiSetCss (GtkWidget *w, char *style);
void  uiSetUIFont (char *uifont);
void  uiInitUILog (void);
void  uiGetForegroundColor (GtkWidget *widget, char *buff, size_t sz);
void  uiWidgetDisable (GtkWidget *widget);
void  uiWidgetEnable (GtkWidget *widget);
void  uiWidgetExpandHoriz (GtkWidget *widget);
void  uiWidgetExpandVert (GtkWidget *widget);
void  uiWidgetSetAllMargins (GtkWidget *widget, int margin);
void  uiWidgetSetMarginTop (GtkWidget *widget, int margin);
void  uiWidgetSetMarginStart (GtkWidget *widget, int margin);
void  uiWidgetAlignHorizFill (GtkWidget *widget);
void  uiWidgetAlignHorizStart (GtkWidget *widget);
void  uiWidgetAlignHorizEnd (GtkWidget *widget);
void  uiWidgetAlignVertFill (GtkWidget *widget);
void  uiWidgetAlignVertStart (GtkWidget *widget);
void  uiWidgetDisableFocus (GtkWidget *widget);
void  uiWidgetHide (GtkWidget *widget);
void  uiWidgetShow (GtkWidget *widget);
void  uiWidgetShowAll (GtkWidget *widget);

/* uigtktoggle.c */
GtkWidget * uiCreateCheckButton (const char *txt, int value);
bool uiToggleButtonIsActive (GtkWidget *widget);

#endif /* INC_UI_H */
