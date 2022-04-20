#ifndef INC_ORGOPT_H
#define INC_ORGOPT_H

#include "datafile.h"
#include "slist.h"

typedef struct {
  datafile_t    *df;
  slist_t       *orgList;
} orgopt_t;

orgopt_t  * orgoptAlloc (char *fname);
void      orgoptFree (orgopt_t *org);
slist_t   * orgoptGetList (orgopt_t *org);

#endif /* INC_ORGOPT_H */
