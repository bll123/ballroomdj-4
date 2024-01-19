/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_SUPPORT_H
#define INC_SUPPORT_H

enum {
  SUPPORT_NO_COMPRESSION,
  SUPPORT_COMPRESSED,
};

typedef struct support support_t;

support_t * supportAlloc (void);
void supportFree (support_t *support);
void supportGetLatestVersion (support_t *support, char *buff, size_t sz);
void supportSendFile (support_t *support, const char *ident, const char *origfn, int compflag);

#endif /* INC_SUPPORT_H */
