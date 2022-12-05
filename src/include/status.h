/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_STATUS_H
#define INC_STATUS_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  STATUS_STATUS,
  STATUS_PLAY_FLAG,
  STATUS_KEY_MAX
} statuskey_t;

typedef struct status status_t;

status_t    * statusAlloc (void);
void        statusFree (status_t *);
ssize_t     statusGetCount (status_t *);
int         statusGetMaxWidth (status_t *);
char        * statusGetStatus (status_t *, ilistidx_t ikey);
ssize_t     statusGetPlayFlag (status_t *status, ilistidx_t ikey);
void        statusStartIterator (status_t *status, ilistidx_t *iteridx);
ilistidx_t  statusIterate (status_t *status, ilistidx_t *iteridx);
void        statusConv (datafileconv_t *conv);
void        statusSave (status_t *status, ilist_t *list);

#endif /* INC_STATUS_H */
