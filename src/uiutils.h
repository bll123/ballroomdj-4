#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

enum {
  UIUTILS_DANCE_COL_IDX,
  UIUTILS_DANCE_COL_NAME,
  UIUTILS_DANCE_COL_MAX,
};

typedef struct {
  GtkWidget     *parentwin;
  GtkWidget     *danceSelectButton;
  GtkWidget     *danceSelectWin;
  GtkWidget     *danceSelect;
  gulong        closeHandlerId;
  bool          danceSelectOpen;
} uiutilsdancesel_t;

#define UI_MAIN_LOOP_TIMER 20

void uiutilsSetCss (GtkWidget *w, char *style);
void uiutilsSetUIFont (char *uifont);
void uiutilsInitGtkLog (void);
GtkWidget * uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata);
GtkWidget * uiutilsCreateScrolledWindow (void);
GtkWidget * uiutilsCreateDropDownButton (char *title, void *clickCallback,
    void *udata);
void uiutilsCreateDropDown (GtkWidget *parentwin, GtkWidget *parentwidget,
    void *closeCallback, void *processSelectionCallback,
    GtkWidget **win, GtkWidget **treeview, void *closeudata, void *udata);
void uiutilsCreateDanceSelect (GtkWidget *parentwin,
    char *label, uiutilsdancesel_t *dancesel,
    void *processSelectionCallback, void *udata);
void uiutilsShowDanceWindow (GtkButton *b, gpointer udata);
void uiutilsCreateDanceList (uiutilsdancesel_t *dancesel);
ssize_t uiutilsGetDanceSelection (uiutilsdancesel_t *dancesel,
    GtkTreePath *path);


#endif /* INC_UIUTILS_H */
