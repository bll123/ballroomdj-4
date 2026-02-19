/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "nodiscard.h"
#include "slist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct podcast podcast_t;

enum {
  PODCAST_IMAGE_URI,
  PODCAST_LAST_BLD_DATE,
  PODCAST_RETAIN,
  PODCAST_TITLE,
  PODCAST_URI,
  PODCAST_KEY_MAX,
};

BDJ_NODISCARD podcast_t *podcastLoad (const char *fname);
BDJ_NODISCARD podcast_t *podcastCreate (const char *fname);
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

