#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "musicdb.h"
#include "musicq.h"
#include "nlist.h"
#include "tmutil.h"
#include "ui.h"

enum {
  UIMUSICQ_SEL_NONE,
  UIMUSICQ_SEL_CURR,
  UIMUSICQ_SEL_PREV,
  UIMUSICQ_SEL_NEXT,
  UIMUSICQ_SEL_TOP,
};

enum {
  UIMUSICQ_REPEAT_NONE,
  UIMUSICQ_REPEAT_UP,
  UIMUSICQ_REPEAT_DOWN,
};

enum {
  UIMUSICQ_PEER_MAX = 2,
  UIMUSICQ_REPEAT_TIME = 250,
};

typedef struct uimusicqgtk uimusicqgtk_t;

typedef struct {
  int           count;          // how many songs displayed in queue
  long          selectLocation;
  /* music queue tab */
  UIWidget      mainbox;
  uidropdown_t  *playlistsel;
  uientry_t     *slname;
  /* widget data */
  uimusicqgtk_t *uiWidgets;
  /* flags */
  bool          hasui : 1;
  bool          haveselloc : 1;
} uimusicqui_t;

typedef struct uimusicq uimusicq_t;

typedef void (*uimusicqiteratecb_t)(uimusicq_t *uimusicq, dbidx_t dbidx);

typedef struct uimusicq {
  const char      *tag;
  int             musicqPlayIdx;    // needed for clear queue
  int             musicqManageIdx;
  conn_t          *conn;
  dispsel_t       *dispsel;
  musicdb_t       *musicdb;
  dispselsel_t    dispselType;
  UIWidget        *parentwin;
  UIWidget        pausePixbuf;
  UICallback      *newselcb;
  UICallback      *editcb;
  UICallback      queueplcb;
  UICallback      queuedancecb;
  uimusicqui_t    ui [MUSICQ_MAX];
  mstime_t        repeatTimer;
  int             repeatButton;
  /* peers */
  int           peercount;
  uimusicq_t    *peers [UIMUSICQ_PEER_MAX];
  bool          ispeercall;
  /* temporary stuff used for music queue update processing */
  nlist_t         *uniqueList;
  nlist_t         *dispList;
  nlist_t         *workList;
  /* temporary for save */
  uimusicqiteratecb_t iteratecb;
  nlist_t         *savelist;
  bool            backupcreated;
  int             cbci;
} uimusicq_t;

typedef struct {
  slistidx_t      dbidx;
  int             dispidx;
  int             uniqueidx;
  int             pflag;
} musicqupdate_t;

/* uimusicq.c */
uimusicq_t  * uimusicqInit (const char *tag, conn_t *conn, musicdb_t *musicdb,
    dispsel_t *dispsel, dispselsel_t dispselType);
void  uimusicqSetPeer (uimusicq_t *uimusicq, uimusicq_t *peer);
void  uimusicqSetDatabase (uimusicq_t *uimusicq, musicdb_t *musicdb);
void  uimusicqFree (uimusicq_t *uimusicq);
void  uimusicqMainLoop (uimusicq_t *uimuiscq);
int   uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
void  uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx);
void  uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);
void  uimusicqSetSelectionCallback (uimusicq_t *uimusicq, UICallback *uicbdbidx);
void  uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm);
const char * uimusicqGetSonglistName (uimusicq_t *uimusicq);
void  uimusicqPeerSonglistName (uimusicq_t *targetqueue, uimusicq_t *sourcequeue);
long  uimusicqGetCount (uimusicq_t *uimusicq);
void  uimusicqSave (uimusicq_t *uimusicq, const char *name);
void  uimusicqSetEditCallback (uimusicq_t *uimusicq, UICallback *uicb);

/* uimusicqgtk.c */
void      uimusicqUIInit (uimusicq_t *uimusicq);
void      uimusicqUIFree (uimusicq_t *uimusicq);
UIWidget  * uimusicqBuildUI (uimusicq_t *uimusicq, UIWidget *parentwin, int ci);
void      uimusicqUIMainLoop (uimusicq_t *uimuiscq);
void      uimusicqSetSelectionFirst (uimusicq_t *uimusicq, int mqidx);
ssize_t   uimusicqGetSelection (uimusicq_t *uimusicq);
void      uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which);
void      uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);
void      uimusicqRemoveHandler (GtkButton *b, gpointer udata);
void      uimusicqIterate (uimusicq_t *uimusicq, uimusicqiteratecb_t cb, musicqidx_t mqidx);
long      uimusicqGetSelectLocation (uimusicq_t *uimusicq, int mqidx);
void      uimusicqSetSelectLocation (uimusicq_t *uimusicq, int mqidx, long loc);
bool      uimusicqClearQueueCallback (void *udata);

/* uimusicqcommon.c */
void  uimusicqQueueDanceProcess (uimusicq_t *uimusicq, ssize_t idx);
void  uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, ssize_t idx);
bool  uimusicqStopRepeat (void *udata);
void  uimusicqMoveTop (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqMoveUp (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqMoveDown (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqTogglePause (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqRemove (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
void  uimusicqClearQueue (uimusicq_t *uimusicq, int mqidx, long idx);
void  uimusicqPlay (uimusicq_t *uimusicq, int mqidx, dbidx_t dbidx);
int   uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char * args);
void  uimusicqMusicQueueDataFree (uimusicq_t *uimusicq);
void  uimusicqSetPeerFlag (uimusicq_t *uimusicq, bool val);

#endif /* INC_UIMUSICQ_H */

