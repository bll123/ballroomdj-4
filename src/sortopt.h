#ifndef INC_SORTOPT_H
#define INC_SORTOPT_H

#include "datafile.h"

typedef struct {
  datafile_t        *df;
} sortopt_t;

sortopt_t     *sortoptAlloc (char *);
void          sortoptFree (sortopt_t *);

#endif /* INC_SORTOPT_H */
