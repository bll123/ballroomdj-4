/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_EXPIMP_H
#define INC_EXPIMP_H

#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* m3u.c */
void m3uExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
nlist_t * m3uImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz);

/* jspf.c */
void jspfExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
nlist_t * jspfImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz);

/* xspf.c */
void xspfExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
nlist_t * xspfImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_EXPIMP_H */
