/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ASCONF_H
#define INC_ASCONF_H

#include "nodiscard.h"
#include "ilist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  ASCONF_MODE_OFF,
  ASCONF_MODE_CLIENT,
  ASCONF_MODE_SERVER,
  ASCONF_MODE_MAX,
};

typedef enum {
  ASCONF_MODE,
  ASCONF_TYPE,
  ASCONF_NAME,
  ASCONF_URI,
  ASCONF_PORT,
  ASCONF_USER,
  ASCONF_PASS,
  ASCONF_KEY_MAX,
} asconfkey_t;

enum {
  ASCONF_CURR_VERSION = 1,
};

typedef struct asconf asconf_t;

NODISCARD asconf_t *asconfAlloc (void);
void asconfFree (asconf_t *);
ssize_t asconfGetCount (asconf_t *);
const char * asconfGetStr (asconf_t *, ilistidx_t key, ilistidx_t idx);
ssize_t asconfGetNum (asconf_t *, ilistidx_t key, ilistidx_t idx);
void asconfSetStr (asconf_t *, ilistidx_t key, ilistidx_t idx, const char *str);
void asconfSetNum (asconf_t *, ilistidx_t key, ilistidx_t idx, ssize_t value);
void asconfSave (asconf_t *asconf, ilist_t *list, int newdistvers);
void asconfDelete (asconf_t *asconf, ilistidx_t key);
ilistidx_t asconfAdd (asconf_t *asconf, const char *name);

/* operate on the sorted list */
void asconfStartIterator (asconf_t *, slistidx_t *idx);
ilistidx_t asconfIterate (asconf_t *, slistidx_t *idx);
ilistidx_t asconfGetListASKey (asconf_t *, slistidx_t idx);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ASCONF_H */
