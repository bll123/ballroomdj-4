/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UI_H
#define INC_UI_H

#include <stdbool.h>

#if BDJ4_USE_GTK
# include <gtk/gtk.h>
#endif

#include "nlist.h"
#include "slist.h"
#include "song.h"
#include "tmutil.h"
#include "callback.h"

#if BDJ4_USE_GTK
/* these are defined based on the gtk values */
/* would change for a different gui package */
enum {
  UICB_STOP = true,
  UICB_CONT = false,
  UICB_DISPLAYED = true,
  UICB_NO_DISP = false,
  UICB_NO_CONV = false,
  UICB_CONVERTED = true,
  SELECT_SINGLE = GTK_SELECTION_SINGLE,
  SELECT_MULTIPLE = GTK_SELECTION_MULTIPLE,
  UICB_SUPPORTED = true,
  UICB_NOT_SUPPORTED = false,
};
#endif

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

#define ACCENT_CLASS "accent"
#define DARKACCENT_CLASS "darkaccent"
#define ERROR_CLASS "error"

/* uigtkutils.c */
extern int uiBaseMarginSz;

/* uigeneral.c */
/* general routines that are called by the ui specific code */
void uiutilsUIWidgetInit (UIWidget *uiwidget);
bool uiutilsUIWidgetSet (UIWidget *uiwidget);
void uiutilsUIWidgetCopy (UIWidget *target, UIWidget *source);

/* uigtkdialog.c */
typedef struct uiselect uiselect_t;

enum {
  RESPONSE_NONE,
  RESPONSE_DELETE_WIN,
  RESPONSE_CLOSE,
  RESPONSE_APPLY,
  RESPONSE_RESET,
};

char  *uiSelectDirDialog (uiselect_t *selectdata);
char  *uiSelectFileDialog (uiselect_t *selectdata);
char  *uiSaveFileDialog (uiselect_t *selectdata);
void uiCreateDialog (UIWidget *uiwidget, UIWidget *window,
    callback_t *uicb, const char *title, ...);
void  uiDialogPackInDialog (UIWidget *uidialog, UIWidget *boxp);
void  uiDialogDestroy (UIWidget *uidialog);
uiselect_t *uiDialogCreateSelect (UIWidget *window, const char *label, const char *startpath, const char *dfltname, const char *mimefiltername, const char *mimetype);

/* uigtkkeys.c */
typedef struct uikey uikey_t;

uikey_t * uiKeyAlloc (void);
void    uiKeyFree (uikey_t *uikey);
void    uiKeySetKeyCallback (uikey_t *uikey, UIWidget *uiwidgetp, callback_t *uicb);
int     uiKeyEvent (uikey_t *uikey);
bool    uiKeyIsPressEvent (uikey_t *uikey);
bool    uiKeyIsReleaseEvent (uikey_t *uikey);
bool    uiKeyIsMovementKey (uikey_t *uikey);
bool    uiKeyIsNKey (uikey_t *uikey);
bool    uiKeyIsPKey (uikey_t *uikey);
bool    uiKeyIsSKey (uikey_t *uikey);
bool    uiKeyIsAudioPlayKey (uikey_t *uikey);
bool    uiKeyIsAudioPauseKey (uikey_t *uikey);
bool    uiKeyIsAudioStopKey (uikey_t *uikey);
bool    uiKeyIsAudioNextKey (uikey_t *uikey);
bool    uiKeyIsAudioPrevKey (uikey_t *uikey);
bool    uiKeyIsUpKey (uikey_t *uikey);
bool    uiKeyIsDownKey (uikey_t *uikey);
bool    uiKeyIsPageUpDownKey (uikey_t *uikey);
bool    uiKeyIsNavKey (uikey_t *uikey);
bool    uiKeyIsMaskedKey (uikey_t *uikey);
bool    uiKeyIsControlPressed (uikey_t *uikey);
bool    uiKeyIsShiftPressed (uikey_t *uikey);

/* uigtkmenu.c */
typedef struct {
  int             menucount;
  UIWidget        menuitem [UIUTILS_MENU_MAX];
  bool            initialized : 1;
} uimenu_t;

void uiCreateMenubar (UIWidget *uiwidget);
void uiCreateSubMenu (UIWidget *uimenuitem, UIWidget *uimenu);
void uiMenuCreateItem (UIWidget *uimenu, UIWidget *uimenuitem, const char *txt, callback_t *uicb);
void uiMenuCreateCheckbox (UIWidget *uimenu, UIWidget *uimenuitem,
    const char *txt, int active, callback_t *uicb);
void uiMenuInit (uimenu_t *menu);
void uiMenuAddMainItem (UIWidget *uimenubar, UIWidget *uimenuitem,
    uimenu_t *menu, const char *txt);
void uiMenuAddSeparator (UIWidget *uimenu, UIWidget *uimenuitem);
void uiMenuSetMainCallback (UIWidget *uimenuitem, callback_t *uicb);
void uiMenuDisplay (uimenu_t *menu);
void uiMenuClear (uimenu_t *menu);

/* uigtklabel.c */
void  uiCreateLabel (UIWidget *uiwidget, const char *label);
void  uiCreateColonLabel (UIWidget *uiwidget, const char *label);
void  uiLabelAddClass (const char *classnm, const char *color);
void  uiLabelSetFont (UIWidget *uilabel, const char *font);
void  uiLabelSetBackgroundColor (UIWidget *uiwidget, const char *color);
void  uiLabelSetText (UIWidget *uilabel, const char *text);
const char * uiLabelGetText (UIWidget *uiwidget);
void  uiLabelEllipsizeOn (UIWidget *uiwidget);
void  uiLabelSetSelectable (UIWidget *uiwidget);
void  uiLabelSetMaxWidth (UIWidget *uiwidget, int width);
void  uiLabelAlignEnd (UIWidget *uiwidget);

/* uigtkchgind.c */
typedef struct uichgind uichgind_t;

uichgind_t *uiCreateChangeIndicator (UIWidget *boxp);
void  uichgindFree (uichgind_t *uichgind);
void  uichgindMarkNormal (uichgind_t *uichgind);
void  uichgindMarkError (uichgind_t *uichgind);
void  uichgindMarkChanged (uichgind_t *uichgind);

/* uigtkbutton.c */
typedef struct uibutton uibutton_t;

uibutton_t *uiCreateButton (callback_t *uicb, char *title, char *imagenm);
void uiButtonFree (uibutton_t *uibutton);
UIWidget *uiButtonGetUIWidget (uibutton_t *uibutton);
void uiButtonSetImagePosRight (uibutton_t *uibutton);
void uiButtonSetImage (uibutton_t *uibutton, const char *imagenm, const char *tooltip);
void uiButtonSetImageIcon (uibutton_t *uibutton, const char *nm);
void uiButtonAlignLeft (uibutton_t *uibutton);
void uiButtonSetText (uibutton_t *uibutton, const char *txt);
void uiButtonSetReliefNone (uibutton_t *uibutton);
void uiButtonSetFlat (uibutton_t *uibutton);
void uiButtonSetRepeat (uibutton_t *uibutton, int repeatms);
void uiButtonCheckRepeat (uibutton_t *uibutton);

/* uigtkentry.c */
typedef struct uientry uientry_t;
typedef int (*uientryval_t)(uientry_t *entry, void *udata);

enum {
  UIENTRY_IMMEDIATE,
  UIENTRY_DELAYED,
};

enum {
  UIENTRY_RESET,
  UIENTRY_ERROR,
  UIENTRY_OK,
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
void uiEntrySetValidate (uientry_t *entry,
    uientryval_t valfunc, void *udata, int valdelay);
int uiEntryValidate (uientry_t *entry, bool forceflag);
int uiEntryValidateDir (uientry_t *edata, void *udata);
int uiEntryValidateFile (uientry_t *edata, void *udata);
void uiEntryDisable (uientry_t *entry);
void uiEntryEnable (uientry_t *entry);

/* uigtkspinbox.c */
enum {
  SB_TEXT,
  SB_TIME_BASIC,
  SB_TIME_PRECISE,
};

typedef const char * (*uispinboxdisp_t)(void *, int);
typedef struct uispinbox uispinbox_t;

uispinbox_t *uiSpinboxInit (void);
void  uiSpinboxFree (uispinbox_t *spinbox);

void  uiSpinboxTextCreate (uispinbox_t *spinbox, void *udata);
void  uiSpinboxTextSet (uispinbox_t *spinbox, int min, int count,
    int maxWidth, slist_t *list, nlist_t *keylist, uispinboxdisp_t textGetProc);
int   uiSpinboxTextGetIdx (uispinbox_t *spinbox);
int   uiSpinboxTextGetValue (uispinbox_t *spinbox);
void  uiSpinboxTextSetValue (uispinbox_t *spinbox, int ivalue);
void  uiSpinboxDisable (uispinbox_t *spinbox);
void  uiSpinboxEnable (uispinbox_t *spinbox);

void  uiSpinboxIntCreate (UIWidget *uiwidget);
void  uiSpinboxDoubleCreate (UIWidget *uiwidget);

void  uiSpinboxDoubleDefaultCreate (uispinbox_t *spinbox);

uispinbox_t *uiSpinboxTimeInit (int sbtype);
void  uiSpinboxTimeCreate (uispinbox_t *spinbox, void *udata, callback_t *convcb);
ssize_t uiSpinboxTimeGetValue (uispinbox_t *spinbox);
void  uiSpinboxTimeSetValue (uispinbox_t *spinbox, ssize_t value);

void  uiSpinboxSetRange (uispinbox_t *spinbox, double min, double max);
void  uiSpinboxWrap (uispinbox_t *spinbox);
void  uiSpinboxSet (UIWidget *uispinbox, double min, double max);
double uiSpinboxGetValue (UIWidget *uispinbox);
void  uiSpinboxSetValue (UIWidget *uispinbox, double ivalue);
bool  uiSpinboxIsChanged (uispinbox_t *spinbox);
void  uiSpinboxResetChanged (uispinbox_t *spinbox);
void  uiSpinboxAlignRight (uispinbox_t *spinbox);
void  uiSpinboxAddClass (const char *classnm, const char *color);
UIWidget * uiSpinboxGetUIWidget (uispinbox_t *spinbox);
void uiSpinboxTextSetValueChangedCallback (uispinbox_t *spinbox, callback_t *uicb);
void uiSpinboxTimeSetValueChangedCallback (uispinbox_t *spinbox, callback_t *uicb);
void uiSpinboxSetValueChangedCallback (UIWidget *uiwidget, callback_t *uicb);

/* uigtkdropdown.c */
typedef struct uidropdown uidropdown_t;

uidropdown_t *uiDropDownInit (void);
void uiDropDownFree (uidropdown_t *dropdown);
UIWidget *uiDropDownCreate (UIWidget *parentwin,
    const char *title, callback_t *uicb,
    uidropdown_t *dropdown, void *udata);
UIWidget *uiComboboxCreate (UIWidget *parentwin,
    const char *title, callback_t *uicb,
    uidropdown_t *dropdown, void *udata);
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
void uiCreateLink (UIWidget *uiwidget, const char *label, const char *uri);
void uiLinkSet (UIWidget *uilink, const char *label, const char *uri);
void uiLinkSetActivateCallback (UIWidget *uilink, callback_t *uicb);

/* uigtkmiscbutton.c */
void uiCreateFontButton (UIWidget *uiwidget, const char *fontname);
const char * uiFontButtonGetFont (UIWidget *uiwidget);
void uiCreateColorButton (UIWidget *uiwidget, const char *color);
void uiColorButtonGetColor (UIWidget *uiwidget, char *tbuff, size_t sz);

/* uigtktextbox.c */
typedef struct uitextbox uitextbox_t;

uitextbox_t  *uiTextBoxCreate (int height, const char *hlcolor);
void  uiTextBoxFree (uitextbox_t *tb);
UIWidget * uiTextBoxGetScrolledWindow (uitextbox_t *tb);
char  *uiTextBoxGetValue (uitextbox_t *tb);
void  uiTextBoxSetReadonly (uitextbox_t *tb);
void  uiTextBoxScrollToEnd (uitextbox_t *tb);
void  uiTextBoxAppendStr (uitextbox_t *tb, const char *str);
void  uiTextBoxAppendBoldStr (uitextbox_t *tb, const char *str);
void  uiTextBoxAppendHighlightStr (uitextbox_t *tb, const char *str);
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
void  uiNotebookSetCallback (UIWidget *uinotebook, callback_t *uicb);
void  uiNotebookHideShowPage (UIWidget *uinotebook, int pagenum, bool show);

/* uigtkbox.c */
void uiCreateVertBox (UIWidget *uiwidget);
void uiCreateHorizBox (UIWidget *uiwidget);
void uiBoxPackInWindow (UIWidget *uiwindow, UIWidget *uibox);
void uiBoxPackStart (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackStartExpand (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackEnd (UIWidget *uibox, UIWidget *uiwidget);
void uiBoxPackEndExpand (UIWidget *uibox, UIWidget *uiwidget);

/* uigtkpbar.c */
void uiCreateProgressBar (UIWidget *uiwidget, char *color);
void uiProgressBarSet (UIWidget *uipb, double val);

/* uigtktreeview.c */

typedef struct uitree uitree_t;

enum {
  UITREE_TYPE_NUM,
  UITREE_TYPE_STRING,
};

uitree_t *uiCreateTreeView (void);
void  uiTreeViewFree (uitree_t *uitree);
UIWidget * uiTreeViewGetUIWidget (uitree_t *uitree);
#if BDJ4_USE_GTK
int   uiTreeViewGetSelection (uitree_t *uitree, GtkTreeModel **model, GtkTreeIter *iter);
#endif
void  uiTreeViewAllowMultiple (uitree_t *uitree);
void  uiTreeViewEnableHeaders (uitree_t *uitree);
void  uiTreeViewDisableHeaders (uitree_t *uitree);
void  uiTreeViewDarkBackground (uitree_t *uitree);
void  uiTreeViewDisableSingleClick (uitree_t *uitree);
void  uiTreeViewSelectionSetMode (uitree_t *uitree, int mode);
void  uiTreeViewSelectionSet (uitree_t *uitree, int row);
int   uiTreeViewSelectionGetCount (uitree_t *uitree);
#if BDJ4_USE_GTK
GtkTreeViewColumn * uiTreeViewAddDisplayColumns (uitree_t *uitree,
    slist_t *sellist, int col, int fontcol, int ellipsizeCol);
GType * uiTreeViewAddDisplayType (GType *types, int valtype, int col);
void  uiTreeViewSetDisplayColumn (GtkTreeModel *model, GtkTreeIter *iter,
    int col, long num, const char *str);
#endif
void  uiTreeViewSetEditedCallback (uitree_t *uitree, callback_t *uicb);
void  uiTreeViewAddEditableColumn (uitree_t *uitree, int col, int editcol, const char *title);

/* uigtkwindow.c */
void uiCreateMainWindow (UIWidget *uiwidget, callback_t *uicb,
    const char *title, const char *imagenm);
void uiWindowSetTitle (UIWidget *uiwidget, const char *title);
void uiCloseWindow (UIWidget *uiwindow);
bool uiWindowIsMaximized (UIWidget *uiwindow);
void uiWindowIconify (UIWidget *uiwindow);
void uiWindowDeIconify (UIWidget *uiwindow);
void uiWindowMaximize (UIWidget *uiwindow);
void uiWindowUnMaximize (UIWidget *uiwindow);
void uiWindowDisableDecorations (UIWidget *uiwindow);
void uiWindowEnableDecorations (UIWidget *uiwindow);
void uiWindowGetSize (UIWidget *uiwindow, int *x, int *y);
void uiWindowSetDefaultSize (UIWidget *uiwindow, int x, int y);
void uiWindowGetPosition (UIWidget *uiwindow, int *x, int *y, int *ws);
void uiWindowMove (UIWidget *uiwindow, int x, int y, int ws);
void uiWindowMoveToCurrentWorkspace (UIWidget *uiwindow);
void uiWindowNoFocusOnStartup (UIWidget *uiwindow);
void uiCreateScrolledWindow (UIWidget *uiwindow, int minheight);
void uiWindowSetPolicyExternal (UIWidget *uisw);
void uiCreateDialogWindow (UIWidget *uiwidget, UIWidget *parentwin, UIWidget *attachment, callback_t *uicb, const char *title);
void uiWindowSetDoubleClickCallback (UIWidget *uiwindow, callback_t *uicb);
void uiWindowSetWinStateCallback (UIWidget *uiwindow, callback_t *uicb);
void uiWindowNoDim (UIWidget *uiwidget);
void uiWindowSetMappedCallback (UIWidget *uiwidget, callback_t *uicb);
void uiWindowPresent (UIWidget *uiwidget);
void uiWindowRaise (UIWidget *uiwidget);
void uiWindowFind (UIWidget *window);

/* uigtkscale.c */
void    uiCreateScale (UIWidget *uiwidget, double lower, double upper,
    double stepinc, double pageinc, double initvalue, int digits);
void    uiScaleSetCallback (UIWidget *uiscale, callback_t *uicb);
double  uiScaleEnforceMax (UIWidget *uiscale, double value);
double  uiScaleGetValue (UIWidget *uiscale);
int     uiScaleGetDigits (UIWidget *uiscale);
void    uiScaleSetValue (UIWidget *uiscale, double value);
void    uiScaleSetRange (UIWidget *uiscale, double start, double end);

/* uigtkswitch.c */
typedef struct uiswitch uiswitch_t;

uiswitch_t *uiCreateSwitch (int value);
void uiSwitchFree (uiswitch_t *uiswitch);
void uiSwitchSetValue (uiswitch_t *uiswitch, int value);
int uiSwitchGetValue (uiswitch_t *uiswitch);
UIWidget *uiSwitchGetUIWidget (uiswitch_t *uiswitch);
void uiSwitchSetCallback (uiswitch_t *uiswitch, callback_t *uicb);
void uiSwitchDisable (uiswitch_t *uiswitch);
void uiSwitchEnable (uiswitch_t *uiswitch);

/* uigtksizegrp.c */
void uiCreateSizeGroupHoriz (UIWidget *);
void uiSizeGroupAdd (UIWidget *uiw, UIWidget *uiwidget);

/* uigtkutils.c */
void  uiUIInitialize (void);
void  uiUIProcessEvents (void);
void  uiUIProcessWaitEvents (void);     // a small delay
void  uiCleanup (void);
#if BDJ4_USE_GTK
void  uiSetCss (GtkWidget *w, const char *style);
#endif
void  uiSetUIFont (const char *uifont, const char *accentcolor, const char *errorcolor);
void  uiAddColorClass (const char *classnm, const char *color);
void  uiAddBGColorClass (const char *classnm, const char *color);
void  uiInitUILog (void);

/* uigtkwidget.c */
/* widget interface */
void  uiWidgetDisable (UIWidget *uiwidget);
void  uiWidgetEnable (UIWidget *uiwidget);
void  uiWidgetExpandHoriz (UIWidget *uiwidget);
void  uiWidgetExpandVert (UIWidget *uiwidget);
void  uiWidgetSetAllMargins (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginTop (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginBottom (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginStart (UIWidget *uiwidget, int mult);
void  uiWidgetSetMarginEnd (UIWidget *uiwidget, int mult);
void  uiWidgetAlignHorizFill (UIWidget *uiwidget);
void  uiWidgetAlignHorizStart (UIWidget *uiwidget);
void  uiWidgetAlignHorizEnd (UIWidget *uiwidget);
void  uiWidgetAlignHorizCenter (UIWidget *uiwidget);
void  uiWidgetAlignVertFill (UIWidget *uiwidget);
void  uiWidgetAlignVertStart (UIWidget *uiwidget);
void  uiWidgetAlignVertCenter (UIWidget *uiwidget);
void  uiWidgetDisableFocus (UIWidget *uiwidget);
void  uiWidgetHide (UIWidget *uiwidget);
void  uiWidgetShow (UIWidget *uiwidget);
void  uiWidgetShowAll (UIWidget *uiwidget);
void  uiWidgetMakePersistent (UIWidget *uiuiwidget);
void  uiWidgetClearPersistent (UIWidget *uiuiwidget);
void  uiWidgetSetSizeRequest (UIWidget *uiuiwidget, int width, int height);
bool  uiWidgetIsValid (UIWidget *uiwidget);
void  uiWidgetGetPosition (UIWidget *widget, int *x, int *y);
void  uiWidgetSetClass (UIWidget *uiwidget, const char *class);
void  uiWidgetRemoveClass (UIWidget *uiwidget, const char *class);

/* uigtkimage.c */
void  uiImageFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFromFile (UIWidget *uiwidget, const char *fn);
void  uiImagePersistentFree (UIWidget *uiwidget);

/* uigtktoggle.c */
void uiCreateCheckButton (UIWidget *uiwidget, const char *txt, int value);
void uiCreateRadioButton (UIWidget *uiwidget, UIWidget *widgetgrp, const char *txt, int value);
void uiCreateToggleButton (UIWidget *uiwidget, const char *txt, const char *imgname,
    const char *tooltiptxt, UIWidget *image, int value);
void uiToggleButtonSetCallback (UIWidget *uiwidget, callback_t *uicb);
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

/* uigtkscrollbar.c */
void uiCreateVerticalScrollbar (UIWidget *uiwidget, double upper);
void uiScrollbarSetUpper (UIWidget *uisb, double upper);
void uiScrollbarSetPosition (UIWidget *uisb, double pos);
void uiScrollbarSetStepIncrement (UIWidget *uisb, double step);
void uiScrollbarSetPageIncrement (UIWidget *uisb, double page);
void uiScrollbarSetPageSize (UIWidget *uisb, double sz);

/* uigtksep.c */
void uiCreateHorizSeparator (UIWidget *uiwidget);
void uiSeparatorAddClass (const char *classnm, const char *color);

/* uigtkclipboard.c */
void uiClipboardSet (const char *txt);

#endif /* INC_UI_H */
