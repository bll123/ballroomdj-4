#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "musicdb.h"
#include "progstate.h"
#include "songfilter.h"
#include "uiutils.h"

typedef struct {
  progstate_t       *progstate;
  conn_t            *conn;
  int               maxRows;
  int               lastTreeSize;
  ssize_t           idxStart;
  double            ddbcount;
  songfilter_t      *songfilter;
  /* song selection tab */
  uiutilsdancesel_t dancesel;
  GtkWidget         *vbox;
  GtkWidget         *songselTree;
  GtkWidget         *songselScrollbar;
  GtkEventController  *scrollController;
  GtkTreeViewColumn   * favColumn;
  /* internal flags */
  bool              createRowProcessFlag : 1;
  bool              createRowFlag : 1;
} uisongsel_t;

uisongsel_t * uisongselInit (progstate_t *progstate, conn_t *conn);
void        uisongselFree (uisongsel_t *uisongsel);
GtkWidget   * uisongselActivate (uisongsel_t *uisongsel, GtkWidget *parentwin);
void        uisongselMainLoop (uisongsel_t *uisongsel);

#endif /* INC_UISONGSEL_H */

