/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_BDJ4ARG_H
#define INC_BDJ4ARG_H

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct bdj4arg bdj4arg_t;

[[nodiscard]] bdj4arg_t *bdj4argInit (int argc, char *argv []);
void bdj4argCleanup (bdj4arg_t *bdj4arg);
char **bdj4argGetArgv (bdj4arg_t *bdj4arg);
const char * bdj4argGet (bdj4arg_t *bdj4arg, int idx, const char *arg);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_BDJ4ARG_H */
