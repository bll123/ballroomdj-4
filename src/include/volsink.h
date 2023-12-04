/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VOLSINK_H
#define INC_VOLSINK_H

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

#endif /* INC_VOLSINK_H */
