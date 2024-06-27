/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>

#include "callback.h"
#include "conn.h"
#include "dispsel.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "uiwcont.h"
#include "ui/uientry.h"

enum {
  UIMUSICQ_SEL_NONE,
  UIMUSICQ_SEL_CURR,
  UIMUSICQ_SEL_PREV,
  UIMUSICQ_SEL_NEXT,
  UIMUSICQ_SEL_TOP,
  MUSICQ_NEW_DISP,
  MUSICQ_UPD_DISP,
};

enum {
  UIMUSICQ_PEER_MAX = 2,
};

enum {
  UIMUSICQ_CB_QUEUE_PLAYLIST,
  UIMUSICQ_CB_QUEUE_DANCE,
  UIMUSICQ_CB_SAVE_LIST,
  UIMUSICQ_CB_MAX,
};

/* these are copies of the callbacks and should not be freed */
enum {
  UIMUSICQ_CBC_CLEAR_QUEUE,
  UIMUSICQ_CBC_EDIT,
  UIMUSICQ_CBC_ITERATE,        // temporary for save
  UIMUSICQ_CBC_NEW_SEL,
  UIMUSICQ_CBC_QUEUE,
  UIMUSICQ_CBC_SONG_SAVE,
  UIMUSICQ_CBC_MAX,
};

typedef struct mq_internal mq_internal_t;

typedef struct {
  int           count;          // how many songs displayed in queue
  dispselsel_t  dispselType;
  nlistidx_t    selectLocation;
  nlistidx_t    lastLocation;
  nlistidx_t    prevSelection;
  nlistidx_t    currSelection;
  /* music queue tab */
  uiwcont_t     *mainbox;
  uiwcont_t     *playlistsel;
  uiwcont_t     *slname;
  /* widget data */
  mq_internal_t *mqInternalData;
  /* flags */
  bool          hasui : 1;
  bool          haveselloc : 1;
  bool          selchgbypass : 1;
  bool          newflag : 1;
} uimusicqui_t;

typedef struct uimusicq uimusicq_t;

typedef struct uimusicq {
  const char        *tag;
  int               musicqPlayIdx;    // needed for clear queue
  int               musicqManageIdx;
  conn_t            *conn;
  dispsel_t         *dispsel;
  musicdb_t         *musicdb;
  uiwcont_t         *parentwin;
  uiwcont_t         *pausePixbuf;
  uiwcont_t         *errorMsg;
  uiwcont_t         *statusMsg;
  callback_t        *callbacks [UIMUSICQ_CB_MAX];
  callback_t        *cbcopy [UIMUSICQ_CBC_MAX];
  uimusicqui_t      ui [MUSICQ_MAX];
  /* peers */
  int               peercount;
  uimusicq_t        *peers [UIMUSICQ_PEER_MAX];
  bool              ispeercall;
  /* temporary for save */
  nlist_t           *savelist;
  int               cbci;
  bool              backupcreated : 1;
  bool              changed : 1;
} uimusicq_t;

/* uimusicq.c */
uimusicq_t  * uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType);
void  uimusicqSetPeer (uimusicq_t *uimusicq, uimusicq_t *peer);
void  uimusicqSetDatabase (uimusicq_t *uimusicq, musicdb_t *musicdb);
void  uimusicqFree (uimusicq_t *uimusicq);
void  uimusicqMainLoop (uimusicq_t *uimuiscq);
void  uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx);
void  uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);
void  uimusicqSetSelectionCallback (uimusicq_t *uimusicq, callback_t *uicbdbidx);
void  uimusicqSetSongSaveCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqSetClearQueueCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm);
char * uimusicqGetSonglistName (uimusicq_t *uimusicq);
void  uimusicqPeerSonglistName (uimusicq_t *targetqueue, uimusicq_t *sourcequeue);
nlistidx_t uimusicqGetCount (uimusicq_t *uimusicq);
void  uimusicqSave (uimusicq_t *uimusicq, const char *name);
void  uimusicqSetEditCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqExport (uimusicq_t *uimusicq, const char *fname, const char *slname, int exptype);
void  uimusicqProcessSongSelect (uimusicq_t *uimusicq, mp_songselect_t *songselect);
void  uimusicqSetQueueCallback (uimusicq_t *uimusicq, callback_t *uicb);

/* uimusicqui.c */
void      uimusicqUIInit (uimusicq_t *uimusicq);
void      uimusicqUIFree (uimusicq_t *uimusicq);
uiwcont_t   * uimusicqBuildUI (uimusicq_t *uimusicq, uiwcont_t *parentwin, int ci, uiwcont_t *errorMsg, uiwcont_t *statusMsg, uientryval_t validateFunc);
void      uimusicqDragDropSetURICallback (uimusicq_t *uimusicq, int ci, callback_t *cb);
void      uimusicqUIMainLoop (uimusicq_t *uimuiscq);
void      uimusicqSetSelectionFirst (uimusicq_t *uimusicq, int mqidx);
void      uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which);
nlistidx_t uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx);
void      uimusicqSetSelectLocation (uimusicq_t *uimusicq, int mqidx, nlistidx_t loc);
dbidx_t   uimusicqGetSelectionDbidx (uimusicq_t *uimusicq);
bool      uimusicqTruncateQueueCallback (void *udata);
void      uimusicqSetPlayButtonState (uimusicq_t *uimusicq, int active);
void      uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate);
void      uimusicqSetRequestLabel (uimusicq_t *uimusicq, const char *txt);
nlist_t * uimusicqGetDBIdxList (uimusicq_t *uimusicq, musicqidx_t mqidx);

/* uimusicqcommon.c */
void  uimusicqQueueDanceProcess (uimusicq_t *uimusicq, nlistidx_t idx, int count);
void  uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, nlistidx_t idx);
void  uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqRemove (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqSwap (uimusicq_t *uimusicq, int mqidx);
void  uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
void  uimusicqTruncateQueue (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqPlay (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx);
void  uimusicqQueue (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx);
void  uimusicqSetPeerFlag (uimusicq_t *uimusicq, bool val);
bool  uimusicqSaveListCallback (void *udata, int32_t dbidx);

#endif /* INC_UIMUSICQ_H */

