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

enum {
  PODCAST_PASSWORD,
  PODCAST_RETAIN,
  PODCAST_URI,
  PODCAST_USER,
  PODCAST_KEY_MAX,
};

podcast_t *podcastLoad (const char *fname);
podcast_t *podcastCreate (const char *fname);
void      podcastFree (podcast_t *podcast);
bool      podcastExists (const char *name);
void      podcastSetName (podcast_t *podcast, const char *newname);
void      podcastSave (podcast_t *podcast);
void      podcastSetNum (podcast_t *podcast, int key, ssize_t value);
void      podcastSetStr (podcast_t *podcast, int key, const char *str);
ssize_t   podcastGetNum (podcast_t *podcast, int key);
const char *podcastGetStr (podcast_t *podcast, int key);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_PODCAST_H */
