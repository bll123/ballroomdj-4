#ifndef INC_SORTOPT_H
#define INC_SORTOPT_H

#include "datafile.h"
#include "slist.h"

typedef struct {
  datafile_t      *df;
  slist_t         *sortoptList;
} sortopt_t;

sortopt_t     *sortoptAlloc (void);
void          sortoptFree (sortopt_t *);
slist_t       *sortoptGetList (sortopt_t *);

#endif /* INC_SORTOPT_H */
