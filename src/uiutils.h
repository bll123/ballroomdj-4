#ifndef INC_UIUTILS_H
#define INC_UIUTILS_H

#define UI_MAIN_LOOP_TIMER 20

void uiutilsSetCss (GtkWidget *w, char *style);
void uiutilsSetUIFont (char *uifont);
void uiutilsInitGtkLog (void);
GtkWidget * uiutilsCreateButton (char *title, char *imagenm,
    void *clickCallback, void *udata);
GtkWidget * uiutilsCreateDropDownButton (char *title, void *clickCallback,
    void *udata);
void uiutilsCreateDropDown (GtkWidget *parentwin, GtkWidget *parentwidget,
    void *closeCallback, void *processSelectionCallback,
    GtkWidget **win, GtkWidget **treeview, void *udata);

#endif /* INC_UIUTILS_H */
