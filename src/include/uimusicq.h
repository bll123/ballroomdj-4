/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "callback.h"
#include "conn.h"
#include "dispsel.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "uiplaylist.h"
#include "uigeneral.h"
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
  UIMUSICQ_CB_QUEUE_PLAYLIST,
  UIMUSICQ_CB_QUEUE_DANCE,
  UIMUSICQ_CB_MAX,
};

/* these are copies of the callbacks and should not be freed */
enum {
  UIMUSICQ_USER_CB_CLEAR_QUEUE,
  UIMUSICQ_USER_CB_EDIT,
  UIMUSICQ_USER_CB_NEW_SEL,
  UIMUSICQ_USER_CB_QUEUE,
  UIMUSICQ_USER_CB_SONG_SAVE,
  UIMUSICQ_USER_CB_MAX,
};

typedef struct mq_internal mq_internal_t;

typedef struct {
  int32_t       rowcount;          // how many songs displayed in queue
  dispselsel_t  dispselType;
  nlistidx_t    selectLocation;
  nlistidx_t    lastLocation;
  nlistidx_t    prevSelection;
  nlistidx_t    currSelection;
  /* music queue tab */
  uiwcont_t     *mainbox;
  uiplaylist_t  *playlistsel;
  uiwcont_t     *slname;
  /* widget data */
  mq_internal_t *mqInternalData;
  /* flags */
  bool          hasui : 1;
  bool          haveselloc : 1;
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
  uiwcont_t         *pauseImage;
  uiwcont_t         *errorMsg;
  uiwcont_t         *statusMsg;
  callback_t        *callbacks [UIMUSICQ_CB_MAX];
  callback_t        *usercb [UIMUSICQ_USER_CB_MAX];
  uimusicqui_t      ui [MUSICQ_MAX];
  /* temporary for save */
  int               cbci;
  bool              backupcreated : 1;
  bool              changed : 1;
} uimusicq_t;

/* uimusicq.c */
uimusicq_t  * uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType);
void  uimusicqSetDatabase (uimusicq_t *uimusicq, musicdb_t *musicdb);
void  uimusicqFree (uimusicq_t *uimusicq);
void  uimusicqMainLoop (uimusicq_t *uimuiscq);
void  uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx);
void  uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);
void  uimusicqSetSelectionCallback (uimusicq_t *uimusicq, callback_t *uicbdbidx);
void  uimusicqSetSongSaveCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqSetClearQueueCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm);
void  uimusicqGetSonglistName (uimusicq_t *uimusicq, char *nm, size_t sz);
bool uimusicqSonglistNameIsNotValid (uimusicq_t *uimusicq);
nlistidx_t uimusicqGetCount (uimusicq_t *uimusicq);
void  uimusicqSetEditCallback (uimusicq_t *uimusicq, callback_t *uicb);
void uimusicqExport (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate, const char *fname, const char *slname, int exptype);
void  uimusicqProcessSongSelect (uimusicq_t *uimusicq, mp_songselect_t *songselect);
void  uimusicqSetQueueCallback (uimusicq_t *uimusicq, callback_t *uicb);
void  uimusicqSave (musicdb_t *musicdb, mp_musicqupdate_t *musicqupdate, const char *name);
nlist_t *uimusicqGetDBIdxList (mp_musicqupdate_t *musicqupdate);

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
dbidx_t   uimusicqGetSelectionDBidx (uimusicq_t *uimusicq);
bool      uimusicqTruncateQueueCallback (void *udata);
void      uimusicqSetPlayButtonState (uimusicq_t *uimusicq, int active);
void      uimusicqSetMusicQueueData (uimusicq_t *uimusicq, mp_musicqupdate_t *musicqupdate);
void      uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, int mqidx);
void      uimusicqSetRequestLabel (uimusicq_t *uimusicq, const char *txt);
void uimusicqCopySelectList (uimusicq_t *uimusicq, uimusicq_t *peer);

/* uimusicqcommon.c */
void  uimusicqQueueDanceProcess (uimusicq_t *uimusicq, nlistidx_t idx, int count);
void  uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, const char *fn);
void  uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqRemove (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqSwap (uimusicq_t *uimusicq, int mqidx);
void  uimusicqCreatePlaylistNames (uimusicq_t *uimusicq);
void  uimusicqTruncateQueue (uimusicq_t *uimusicq, int mqidx, nlistidx_t idx);
void  uimusicqPlay (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif
