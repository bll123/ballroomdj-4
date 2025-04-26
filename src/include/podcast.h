/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PODCAST_H
#define INC_PODCAST_H

#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct podcast podcast_t;

podcast_t    *podcastLoad (const char *fname);
podcast_t    *podcastCreate (const char *fname);
void          podcastFree (podcast_t *podcast);
bool          podcastExists (const char *name);
void          podcastSave (podcast_t *podcast, slist_t *slist);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PODCAST_H */
