#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

enum {
  UIUTILS_DROPDOWN_COL_IDX,
  UIUTILS_DROPDOWN_COL_STR,
  UIUTILS_DROPDOWN_COL_DISP,
  UIUTILS_DROPDOWN_COL_MAX,
};

typedef struct {
  GtkWidget     *parentwin;
  GtkWidget     *button;
  GtkWidget     *window;
  GtkWidget     *tree;
  gulong        closeHandlerId;
  char          *strSelection;
  bool          open : 1;
  bool          iscombobox : 1;
} uiutilsdropdown_t;

typedef struct {
  char            *data;
  GtkEntryBuffer  *buffer;
  GtkWidget       *entry;
  int             entrySize;
  int             maxSize;
} uiutilsentry_t;

typedef struct {
  GtkWidget       *spinbox;
} uiutilsspinbox_t;

#define UI_MAIN_LOOP_TIMER 20

void uiutilsSetCss (GtkWidget *w, char *style);
void uiutilsSetUIFont (char *uifont);
void uiutilsInitGtkLog (void);
GtkWidget * uiutilsCreateColonLabel (char *label);
GtkWidget * uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata);
GtkWidget * uiutilsCreateScrolledWindow (void);

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
void uiutilsDropDownSetList (uiutilsdropdown_t *dropdown, slist_t *list);
void uiutilsDropDownSetNumList (uiutilsdropdown_t *dropdown, slist_t *list);

void uiutilsEntryInit (uiutilsentry_t *entry, int entrySize, int maxSize);
void uiutilsEntryFree (uiutilsentry_t *entry);
GtkWidget * uiutilsEntryCreate (uiutilsentry_t *entry);
const char * uiutilsEntryGetValue (uiutilsentry_t *entry);
void uiutilsEntrySetValue (uiutilsentry_t *entry, char *value);

void uiutilsSpinboxInit (uiutilsspinbox_t *spinbox);
void uiutilsSpinboxFree (uiutilsspinbox_t *spinbox);
GtkWidget * uiutilsSpinboxCreate (uiutilsspinbox_t *spinbox);

void uiutilsCreateDanceList (uiutilsdropdown_t *dancesel);

#endif /* INC_UIUTILS_H */
