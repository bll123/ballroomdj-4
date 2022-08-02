#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>

#include <gtk/gtk.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjvarsdf.h"
#include "configui.h"
#include "log.h"
#include "ui.h"

void
confuiDanceSet (GtkListStore *store, GtkTreeIter *iter,
    char *dancedisp, ilistidx_t key)
{
  logProcBegin (LOG_PROC, "confuiDanceSet");
  gtk_list_store_set (store, iter,
      CONFUI_DANCE_COL_DANCE, dancedisp,
      CONFUI_DANCE_COL_SB_PAD, "    ",
      CONFUI_DANCE_COL_DANCE_IDX, (glong) key,
      -1);
  logProcEnd (LOG_PROC, "confuiDanceSet", "");
}

void
confuiGenreSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *genredisp, int clflag)
{
  logProcBegin (LOG_PROC, "confuiGenreSet");
  gtk_list_store_set (store, iter,
      CONFUI_GENRE_COL_EDITABLE, editable,
      CONFUI_GENRE_COL_GENRE, genredisp,
      CONFUI_GENRE_COL_CLASSICAL, clflag,
      -1);
  logProcEnd (LOG_PROC, "confuiGenreSet", "");
}

void
confuiLevelSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *leveldisp, long weight, int def)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "confuiLevelSet");
  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_LEVEL_COL_EDITABLE, editable,
      CONFUI_LEVEL_COL_LEVEL, leveldisp,
      CONFUI_LEVEL_COL_WEIGHT, weight,
      CONFUI_LEVEL_COL_ADJUST, adjustment,
      CONFUI_LEVEL_COL_DIGITS, 0,
      CONFUI_LEVEL_COL_DEFAULT, def,
      -1);
  logProcEnd (LOG_PROC, "confuiLevelSet", "");
}

void
confuiRatingSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *ratingdisp, long weight)
{
  GtkAdjustment     *adjustment;

  logProcBegin (LOG_PROC, "confuiRatingSet");
  adjustment = gtk_adjustment_new (weight, 0.0, 100.0, 1.0, 5.0, 0.0);
  gtk_list_store_set (store, iter,
      CONFUI_RATING_COL_R_EDITABLE, editable,
      CONFUI_RATING_COL_W_EDITABLE, TRUE,
      CONFUI_RATING_COL_RATING, ratingdisp,
      CONFUI_RATING_COL_WEIGHT, weight,
      CONFUI_RATING_COL_ADJUST, adjustment,
      CONFUI_RATING_COL_DIGITS, 0,
      -1);
  logProcEnd (LOG_PROC, "confuiRatingSet", "");
}

void
confuiStatusSet (GtkListStore *store, GtkTreeIter *iter,
    int editable, char *statusdisp, int playflag)
{
  logProcBegin (LOG_PROC, "confuiStatusSet");
  gtk_list_store_set (store, iter,
      CONFUI_STATUS_COL_EDITABLE, editable,
      CONFUI_STATUS_COL_STATUS, statusdisp,
      CONFUI_STATUS_COL_PLAY_FLAG, playflag,
      -1);
  logProcEnd (LOG_PROC, "confuiStatusSet", "");
}


