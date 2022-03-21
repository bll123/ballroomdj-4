#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#include <stdbool.h>

#include "nlist.h"
#include "slist.h"

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
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
  GtkEntryBuffer  *buffer;
  GtkWidget       *entry;
  int             entrySize;
  int             maxSize;
} uiutilsentry_t;

typedef struct {
  GtkWidget             *spinbox;
  int                   curridx;
  uiutilsspinboxdisp_t  textGetProc;
  void                  *udata;
  int                   maxWidth;
  bool                  indisp;
} uiutilsspinbox_t;

#define UI_MAIN_LOOP_TIMER 20

void uiutilsCleanup (void);
void uiutilsSetCss (GtkWidget *w, char *style);
void uiutilsSetUIFont (char *uifont);
void uiutilsInitGtkLog (void);
GtkWidget * uiutilsCreateLabel (char *label);
GtkWidget * uiutilsCreateColonLabel (char *label);
GtkWidget * uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata);
GtkWidget * uiutilsCreateScrolledWindow (void);
GtkWidget * uiutilsCreateSwitch (int value);

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

void uiutilsEntryInit (uiutilsentry_t *entry, int entrySize, int maxSize);
void uiutilsEntryFree (uiutilsentry_t *entry);
GtkWidget * uiutilsEntryCreate (uiutilsentry_t *entry);
const char * uiutilsEntryGetValue (uiutilsentry_t *entry);
void uiutilsEntrySetValue (uiutilsentry_t *entry, char *value);

void uiutilsSpinboxTextInit (uiutilsspinbox_t *spinbox);
void uiutilsSpinboxTextFree (uiutilsspinbox_t *spinbox);
GtkWidget * uiutilsSpinboxTextCreate (uiutilsspinbox_t *spinbox, void *udata);
void uiutilsSpinboxTextSet (uiutilsspinbox_t *spinbox, int min, int count,
    int maxWidth, uiutilsspinboxdisp_t textGetProc);
int   uiutilsSpinboxTextGetValue (uiutilsspinbox_t *spinbox);
void  uiutilsSpinboxTextSetValue (uiutilsspinbox_t *spinbox, int ivalue);

GtkWidget * uiutilsSpinboxIntCreate (void);
GtkWidget * uiutilsSpinboxDoubleCreate (void);

void  uiutilsSpinboxSet (GtkWidget *spinbox, double min, double max);
int   uiutilsSpinboxGetValue (GtkWidget *spinbox);
void  uiutilsSpinboxSetValue (GtkWidget *spinbox, double ivalue);

void uiutilsCreateDanceList (uiutilsdropdown_t *dancesel, char *selectLabel);

#endif /* INC_UIUTILS_H */
