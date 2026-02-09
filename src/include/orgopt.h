/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "datafile.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct {
  datafile_t    *df;
  slist_t       *orgList;
} orgopt_t;

BDJ_NODISCARD orgopt_t  * orgoptAlloc (void);
void      orgoptFree (orgopt_t *org);
slist_t   * orgoptGetList (orgopt_t *org);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

