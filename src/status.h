#ifndef INC_STATUS_H
#define INC_STATUS_H

#include "datafile.h"
#include "list.h"

typedef enum {
  STATUS_LABEL,
  STATUS_PLAY_FLAG,
  STATUS_KEY_MAX
} statuskey_t;

typedef struct {
  datafile_t        *df;
  list_t            *status;
} status_t;

status_t    * statusAlloc (char *);
void        statusFree (status_t *);
bool        statusPlayCheck (status_t *status, listidx_t ikey);
void        statusConv (char *keydata, datafileret_t *ret);

#endif /* INC_STATUS_H */
