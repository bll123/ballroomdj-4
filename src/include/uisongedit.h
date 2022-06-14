#ifndef INC_UISONGEDIT_H
#define INC_UISONGEDIT_H

#include <stdbool.h>

#include "conn.h"
#include "dispsel.h"
#include "musicdb.h"
#include "song.h"
#include "uisongsel.h"
#include "uiutils.h"

typedef struct {
  conn_t            *conn;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  nlist_t           *options;
  void              *uiWidgetData;
  UICallback        *savecb;
} uisongedit_t;

/* uisongedit.c */
uisongedit_t * uisongeditInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel, nlist_t *opts);
void  uisongeditFree (uisongedit_t *uisongedit);
void  uisongeditMainLoop (uisongedit_t *uisongedit);
void  uisongeditNewSelection (uisongedit_t *uisongedit, dbidx_t dbidx);
void  uisongeditSetSaveCallback (uisongedit_t *uisongedit, UICallback *uicb);

/* uisongeditgtk.c */
void  uisongeditUIInit (uisongedit_t *uisongedit);
void  uisongeditUIFree (uisongedit_t *uisongedit);
UIWidget  * uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit, UIWidget *parentwin);
void  uisongeditLoadData (uisongedit_t *uisongedit, song_t *song);
void  uisongeditUIMainLoop (uisongedit_t *uisongedit);

#endif /* INC_UISONGEDIT_H */

