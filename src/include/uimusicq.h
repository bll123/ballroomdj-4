#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "musicq.h"
#include "nlist.h"
#include "uiutils.h"

enum {
  UIMUSICQ_SEL_NONE,
  UIMUSICQ_SEL_CURR,
  UIMUSICQ_SEL_PREV,
  UIMUSICQ_SEL_NEXT,
  UIMUSICQ_SEL_TOP,
};

enum {
  UIMUSICQ_FLAGS_NONE             = 0x0000,
  UIMUSICQ_FLAGS_NO_QUEUE         = 0x0001,
  UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE  = 0x0002,
  UIMUSICQ_FLAGS_NO_REMOVE        = 0x0004,
};

#define UIMUSICQ_REPEAT_TIME 250

typedef struct {
  bool              active;
  guint             repeatTimer;
  /* music queue tab */
  UIWidget          mainbox;
  uidropdown_t dancesel;
  uidropdown_t playlistsel;
  uientry_t    slname;
  /* tree views */
  GtkWidget         *musicqTree;
  GtkTreeSelection  *sel;
  char              *selPathStr;
} uimusicqui_t;

typedef struct uimusicq uimusicq_t;

typedef void (*uimusicqiteratecb_t)(uimusicq_t *uimusicq, dbidx_t dbidx);

typedef struct uimusicq {
  int             musicqPlayIdx;    // needed for clear queue
  int             musicqManageIdx;
  long            count;
  conn_t          *conn;
  dispsel_t       *dispsel;
  musicdb_t       *musicdb;
  dispselsel_t    dispselType;
  int             uimusicqflags;
  GtkWidget       *parentwin;
  UIWidget        pausePixbuf;
  uimusicqui_t    ui [MUSICQ_MAX];
  /* temporary stuff used for music queue update processing */
  nlist_t         *uniqueList;
  nlist_t         *dispList;
  nlist_t         *workList;
  /* temporary for save */
  uimusicqiteratecb_t iteratecb;
  nlist_t         *savelist;
  bool            backupcreated;
} uimusicq_t;

typedef struct {
  slistidx_t      dbidx;
  int             idx;
  int             dispidx;
  int             uniqueidx;
  int             pflag;
} musicqupdate_t;

/* uimusicq.c */
uimusicq_t  * uimusicqInit (conn_t *conn,
    musicdb_t *musicdb, dispsel_t *dispsel,
    int uimusicqflags, dispselsel_t dispselType);
void  uimusicqSetDatabase (uimusicq_t *uimusicq, musicdb_t *musicdb);
void  uimusicqFree (uimusicq_t *uimusicq);
void  uimusicqMainLoop (uimusicq_t *uimuiscq);
int   uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
void  uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx);
void  uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);
void  uimusicqMoveTopProcess (uimusicq_t *uimusicq);
void  uimusicqMoveUpProcess (uimusicq_t *uimusicq);
void  uimusicqMoveDownProcess (uimusicq_t *uimusicq);
void  uimusicqTogglePauseProcess (uimusicq_t *uimusicq);
void  uimusicqRemoveProcess (uimusicq_t *uimusicq);
void  uimusicqStopRepeat (uimusicq_t *uimusicq);
void  uimusicqClearQueueProcess (uimusicq_t *uimusicq);
void  uimusicqQueueDanceProcess (uimusicq_t *uimusicq, ssize_t idx);
void  uimusicqQueuePlaylistProcess (uimusicq_t *uimusicq, ssize_t idx);
void  uimusicqCreatePlaylistList (uimusicq_t *uimusicq);
int   uimusicqMusicQueueDataParse (uimusicq_t *uimusicq, char * args);
void  uimusicqMusicQueueDataFree (uimusicq_t *uimusicq);
void  uimusicqSetSonglistName (uimusicq_t *uimusicq, const char *nm);
const char * uimusicqGetSonglistName (uimusicq_t *uimusicq);
void uimusicqPeerSonglistName (uimusicq_t *targetqueue, uimusicq_t *sourcequeue);
long uimusicqGetCount (uimusicq_t *uimusicq);
void uimusicqSave (uimusicq_t *uimusicq, const char *name);

/* uimusicqgtk.c */
UIWidget  * uimusicqBuildUI (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci);
void      uimusicqSetSelection (uimusicq_t *uimusicq, char *pathstr);
ssize_t   uimusicqGetSelection (uimusicq_t *uimusicq);
void      uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which);
void      uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);
void      uimusicqRemoveProcessSignal (GtkButton *b, gpointer udata);
void      uimusicqIterate (uimusicq_t *uimusicq, uimusicqiteratecb_t cb, musicqidx_t mqidx);

#endif /* INC_UIMUSICQ_H */

