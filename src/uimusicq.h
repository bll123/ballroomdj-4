#ifndef INC_UIMUSICQ_H
#define INC_UIMUSICQ_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "musicq.h"
#include "progstate.h"
#include "uiutils.h"

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
  GtkWidget       *parentwin;
  GdkPixbuf       *pauseImg;
  uimusicqui_t    ui [MUSICQ_MAX];
  /* temporary stuff used for music queue update processing */
  nlist_t         *uniqueList;
  nlist_t         *dispList;
  nlist_t         *workList;
} uimusicq_t;

uimusicq_t  * uimusicqInit (progstate_t *progstate, conn_t *conn);
void        uimusicqFree (uimusicq_t *uimusicq);
GtkWidget   * uimusicqActivate (uimusicq_t *uimusicq, GtkWidget *parentwin, int ci);
void        uimusicqMainLoop (uimusicq_t *uimuiscq);
int         uimusicqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                bdjmsgmsg_t msg, char *args, void *udata);
void        uimusicqSetPlayIdx (uimusicq_t *uimusicq, int playIdx);
void        uimusicqSetManageIdx (uimusicq_t *uimusicq, int manageIdx);

#endif /* INC_UIMUSICQ_H */

