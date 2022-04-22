#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "dispsel.h"
#include "musicq.h"
#include "progstate.h"
#include "uiutils.h"

enum {
  UIMUSICQ_SEL_NONE,
  UIMUSICQ_SEL_PREV,
  UIMUSICQ_SEL_NEXT,
  UIMUSICQ_SEL_TOP,
};

enum {
  UIMUSICQ_FLAGS_NONE             = 0x00,
  UIMUSICQ_FLAGS_NO_QUEUE         = 0x01,
  UIMUSICQ_FLAGS_NO_TOGGLE_PAUSE  = 0x02,
};

#define UIMUSICQ_REPEAT_TIME 250

typedef struct {
  guint             repeatTimer;
  /* music queue tab */
  GtkWidget         *box;
  uiutilsdropdown_t dancesel;
  uiutilsdropdown_t playlistsel;
  /* tree views */
  GtkWidget         *musicqTree;
  char              *selPathStr;
  mstime_t          rowChangeTimer;
} uimusicqui_t;

typedef struct {
  int             musicqPlayIdx;    // needed for clear queue
  int             musicqManageIdx;
  progstate_t     *progstate;
  conn_t          *conn;
  dispsel_t       *dispsel;
  dispselsel_t    dispselType;
  int             uimusicqflags;
  GtkWidget       *parentwin;
  GdkPixbuf       *pauseImg;
  uimusicqui_t    ui [MUSICQ_MAX];
  /* temporary stuff used for music queue update processing */
  nlist_t         *uniqueList;
  nlist_t         *dispList;
  nlist_t         *workList;
} uimusicq_t;

typedef struct {
  slistidx_t      dbidx;
  int             idx;
  int             dispidx;
  int             uniqueidx;
  int             pflag;
} musicqupdate_t;

/* uimusicq.c */
uimusicq_t  * uimusicqInit (progstate_t *progstate,
    conn_t *conn, dispsel_t *dispsel, int uimusicqflags,
    dispselsel_t dispselType);
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

/* uimusicqgtk.c */
GtkWidget * uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci);
void      uimusicqSetSelection (uimusicq_t *uimusicq, char *pathstr);
ssize_t   uimusicqGetSelection (uimusicq_t *uimusicq);
void      uimusicqMusicQueueSetSelected (uimusicq_t *uimusicq, int ci, int which);
void      uimusicqProcessMusicQueueData (uimusicq_t *uimusicq, char * args);

#endif /* INC_UIMUSICQ_H */

