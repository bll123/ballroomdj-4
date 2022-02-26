#ifndef INC_STATUS_H
#define INC_STATUS_H

#include "datafile.h"
#include "ilist.h"

typedef enum {
  STATUS_STATUS,
  STATUS_PLAY_FLAG,
  STATUS_KEY_MAX
} statuskey_t;

typedef struct {
  datafile_t  *df;
  ilist_t     *status;
  int         maxWidth;
} status_t;

status_t    * statusAlloc (char *);
void        statusFree (status_t *);
ssize_t     statusGetCount (status_t *);
int         statusGetMaxWidth (status_t *);
char        * statusGetStatus (status_t *, ilistidx_t ikey);
bool        statusPlayCheck (status_t *status, ilistidx_t ikey);
void        statusConv (char *keydata, datafileret_t *ret);

#endif /* INC_STATUS_H */
