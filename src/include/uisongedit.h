#ifndef INC_UISONGEDIT_H
#define INC_UISONGEDIT_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "musicdb.h"

typedef struct {
  conn_t            *conn;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  nlist_t           *options;
  void              *uiWidgetData;
} uisongedit_t;

/* uisongedit.c */
uisongedit_t * uisongeditInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel, nlist_t *opts);
void  uisongeditFree (uisongedit_t *uisongedit);
void  uisongeditMainLoop (uisongedit_t *uisongedit);

/* uisongeditgtk.c */
void      uisongeditUIInit (uisongedit_t *uisongedit);
void      uisongeditUIFree (uisongedit_t *uisongedit);
GtkWidget * uisongeditActivate (uisongedit_t *uisongedit, GtkWidget *parentwin);

#endif /* INC_UISONGEDIT_H */

