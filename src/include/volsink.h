/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct volsinkitem {
  char      *name;
  char      *description;
  int       defaultFlag;
  int       idxNumber;
  int       nmlen;
} volsinkitem_t;

typedef struct volsinklist {
  char            *defname;
  int             count;
  volsinkitem_t   *sinklist;
} volsinklist_t;

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

