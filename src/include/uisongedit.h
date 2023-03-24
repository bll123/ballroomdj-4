/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UISONGEDIT_H
#define INC_UISONGEDIT_H

#include <stdbool.h>

#include "conn.h"
#include "dispsel.h"
#include "musicdb.h"
#include "song.h"
#include "tmutil.h"
#include "uisongsel.h"
#include "ui.h"

typedef struct {
  conn_t            *conn;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  nlist_t           *options;
  void              *seInternalData;
  uiwcont_t         *statusMsg;
  callback_t        *savecb;
  callback_t        *applyadjcb;
  uisongsel_t       *uisongsel;
} uisongedit_t;

enum {
  UISONGEDIT_EDITALL,
  UISONGEDIT_ALL,
  UISONGEDIT_EDITALL_OFF,
  UISONGEDIT_EDITALL_ON,
};

enum {
  UISONGEDIT_REPEAT_TIME = 200,
};

/* uisongedit.c */
uisongedit_t * uisongeditInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel, nlist_t *opts);
void  uisongeditFree (uisongedit_t *uisongedit);
void  uisongeditMainLoop (uisongedit_t *uisongedit);
int   uisongeditProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
void  uisongeditNewSelection (uisongedit_t *uisongedit, dbidx_t dbidx);
void  uisongeditSetSaveCallback (uisongedit_t *uisongedit, callback_t *uicb);
void  uisongeditSetApplyAdjCallback (uisongedit_t *uisongedit, callback_t *uicb);

/* uisongeditui.c */
void  uisongeditUIInit (uisongedit_t *uisongedit);
void  uisongeditUIFree (uisongedit_t *uisongedit);
uiwcont_t   * uisongeditBuildUI (uisongsel_t *uisongsel, uisongedit_t *uisongedit, uiwcont_t *parentwin, uiwcont_t *statusMsg);
void  uisongeditLoadData (uisongedit_t *uisongedit, song_t *song, dbidx_t dbidx, int editallflag);
void  uisongeditUIMainLoop (uisongedit_t *uisongedit);
void  uisongeditSetBPMValue (uisongedit_t *uisongedit, const char *args);
void  uisongeditSetPlayButtonState (uisongedit_t *uisongedit, int active);
void  uisongeditEditAllSetFields (uisongedit_t *uisongedit, int editflag);
void  uisongeditClearChanged (uisongedit_t *uisongedit, int editallflag);
bool  uisongeditEditAllApply (uisongedit_t *uisongedit);

#endif /* INC_UISONGEDIT_H */

