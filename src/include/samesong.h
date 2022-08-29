#ifndef INC_SAMESONG_H
#define INC_SAMESONG_H

#include "musicdb.h"
#include "nlist.h"

typedef struct samesong samesong_t;
typedef struct sscheck sscheck_t;

samesong_t  *samesongAlloc (musicdb_t *musicdb);
void        samesongFree (samesong_t *ss);
const char  * samesongGetColorByDBIdx (samesong_t *ss, dbidx_t dbidx);
const char  * samesongGetColorBySSIdx (samesong_t *ss, ssize_t ssidx);
void        samesongSet (samesong_t *ss, nlist_t *dbidxlist);
void        samesongClear (samesong_t *ss, nlist_t *dbidxlist);

sscheck_t   *ssCheckAlloc (musicdb_t *musicdb);
void        ssCheckFree (sscheck_t *sschk);
void        ssCheckAdd (sscheck_t *sschk, dbidx_t dbidx);
void        ssCheckRemove (sscheck_t *sschk, dbidx_t dbidx);
bool        ssCheckCheck (sscheck_t *sschk, dbidx_t dbidx);

#endif /* INC_SAMESONG_H */
