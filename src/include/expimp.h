/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <time.h>

#include "nodiscard.h"
#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  RSS_TITLE,
  RSS_URI,
  RSS_COUNT,
  RSS_ITEMS,
  RSS_IDX,
  RSS_BUILD_DATE,
  RSS_IMAGE_URI,
  RSS_MAX,
};

enum {
  RSS_ITEM_DATE,
  RSS_ITEM_DURATION,
  RSS_ITEM_TITLE,
  RSS_ITEM_TYPE,
  RSS_ITEM_URI,
  RSS_ITEM_MAX,
};

/* m3u.c */
void m3uExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
nlist_t * m3uImport (musicdb_t *musicdb, const char *fname);

/* jspf.c */
void jspfExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
NODISCARD nlist_t * jspfImport (musicdb_t *musicdb, const char *fname);

/* xspf.c */
void xspfExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
NODISCARD nlist_t * xspfImport (musicdb_t *musicdb, const char *fname);

/* rss.c */
time_t rssGetUpdateTime (const char *uri);
nlist_t * rssImport (const char *uri);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

