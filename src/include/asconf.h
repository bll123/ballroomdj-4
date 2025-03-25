/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_ASCONF_H
#define INC_ASCONF_H

#include "ilist.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  ASCONF_TYPE_BDJ4,
  ASCONF_TYPE_RTSP,
  ASCONF_TYPE_MAX,
};

typedef enum {
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

asconf_t      *asconfAlloc (void);
void          asconfFree (asconf_t *);
void          asconfStartIterator (asconf_t *, slistidx_t *idx);
ilistidx_t    asconfIterate (asconf_t *, slistidx_t *idx);
ssize_t       asconfGetCount (asconf_t *);
const char *  asconfGetStr (asconf_t *, ilistidx_t dkey, ilistidx_t idx);
ssize_t       asconfGetNum (asconf_t *, ilistidx_t dkey, ilistidx_t idx);
void          asconfSetStr (asconf_t *, ilistidx_t dkey, ilistidx_t idx, const char *str);
void          asconfSetNum (asconf_t *, ilistidx_t dkey, ilistidx_t idx, ssize_t value);
slist_t       *asconfGetAudioSourceList (asconf_t *);
void          asconfSave (asconf_t *asconf, ilist_t *list, int newdistvers);
void          asconfDelete (asconf_t *asconf, ilistidx_t dkey);
ilistidx_t    asconfAdd (asconf_t *asconf, char *name);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_ASCONF_H */
