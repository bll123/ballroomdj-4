/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SAMESONG_H
#define INC_SAMESONG_H

#include "musicdb.h"
#include "nlist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct samesong samesong_t;
typedef struct sscheck sscheck_t;

[[nodiscard]] samesong_t  *samesongAlloc (musicdb_t *musicdb);
void        samesongFree (samesong_t *ss);
const char  * samesongGetColorByDBIdx (samesong_t *ss, dbidx_t dbidx);
const char  * samesongGetColorBySSIdx (samesong_t *ss, ssize_t ssidx);
void        samesongSet (samesong_t *ss, nlist_t *dbidxlist);
void        samesongClear (samesong_t *ss, nlist_t *dbidxlist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_SAMESONG_H */
