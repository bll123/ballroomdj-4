/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJ4ARG_H
#define INC_BDJ4ARG_H

void bdj4argInit (void);
void bdj4argCleanup (void);
char * bdj4argGet (int idx, const char *arg);
void bdj4argClear (void *targ);

#endif /* INC_BDJ4ARG_H */
