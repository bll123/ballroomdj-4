/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_STATUS_H
#define INC_STATUS_H

#include "nodiscard.h"
#include "datafile.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  STATUS_STATUS,
  STATUS_PLAY_FLAG,
  STATUS_KEY_MAX
} statuskey_t;

typedef struct status status_t;

NODISCARD status_t    * statusAlloc (void);
void        statusFree (status_t *);
ilistidx_t  statusGetCount (status_t *);
int         statusGetMaxWidth (status_t *);
const char  * statusGetStatus (status_t *, ilistidx_t ikey);
int         statusGetPlayFlag (status_t *status, ilistidx_t ikey);
void        statusSetStatus (status_t *, ilistidx_t ikey, const char *statusdisp);
void        statusSetPlayFlag (status_t *status, ilistidx_t ikey, int playflag);
void        statusDeleteLast (status_t *status);
void        statusStartIterator (status_t *status, ilistidx_t *iteridx);
ilistidx_t  statusIterate (status_t *status, ilistidx_t *iteridx);
void        statusConv (datafileconv_t *conv);
void        statusSave (status_t *status, ilist_t *list);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_STATUS_H */
