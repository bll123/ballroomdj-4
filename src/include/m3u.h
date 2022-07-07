#ifndef INC_M3U_H
#define INC_M3U_H

#include "musicdb.h"
#include "nlist.h"

void m3uExport (musicdb_t *musicdb, nlist_t *list, const char *fname, const char *slname);
nlist_t * m3uImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz);

#endif /* INC_M3U_H */
