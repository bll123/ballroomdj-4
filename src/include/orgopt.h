/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ORGOPT_H
#define INC_ORGOPT_H

#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct {
  datafile_t    *df;
  slist_t       *orgList;
} orgopt_t;

orgopt_t  * orgoptAlloc (void);
void      orgoptFree (orgopt_t *org);
slist_t   * orgoptGetList (orgopt_t *org);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ORGOPT_H */
