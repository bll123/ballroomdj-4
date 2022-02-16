#ifndef INC_UISONGSEL_H
#define INC_UISONGSEL_H

#include <stdbool.h>
#include <gtk/gtk.h>

#include "conn.h"
#include "musicdb.h"
#include "progstate.h"

typedef struct {
  progstate_t     *progstate;
  conn_t          *conn;
  int             maxRows;
  ssize_t         idxStart;
  int             treeSize;
  int             boxSize;
  double          ddbcount;
  /* song selection tab */
  GtkWidget       *vbox;
  GtkWidget       *songselTree;
  GtkWidget       *songselScrollbar;
  GtkTreeViewColumn * favColumn;
  /* internal flags */
  bool            createRowProcessFlag : 1;
  bool            createRowFlag : 1;
} uisongsel_t;

uisongsel_t * uisongselInit (progstate_t *progstate, conn_t *conn);
void        uisongselFree (uisongsel_t *uisongsel);
GtkWidget   * uisongselActivate (uisongsel_t *uisongsel);
void        uisongselMainLoop (uisongsel_t *uisongsel);

#endif /* INC_UISONGSEL_H */

