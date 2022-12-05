/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VOLSINK_H
#define INC_VOLSINK_H

typedef struct volsinkitem {
  int       defaultFlag;
  int       idxNumber;
  char      *name;
  char      *description;
} volsinkitem_t;

typedef struct volsinklist {
  char            *defname;
  size_t          count;
  volsinkitem_t   *sinklist;
} volsinklist_t;

#endif /* INC_VOLSINK_H */
