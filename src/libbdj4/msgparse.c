/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdjmsg.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "nlist.h"

mp_musicqupdate_t *
msgparseMusicQueueData (char *args)
{
  int               mqidx = 0;
  char              *p;
  char              *tokstr;
  int               idx;
  mp_musicqupditem_t   *musicqupditem;
  mp_musicqupdate_t    *musicqupdate;


  musicqupdate = mdmalloc (sizeof (mp_musicqupdate_t));
  musicqupdate->mqidx = 0;
  musicqupdate->tottime = 0;
  musicqupdate->currdbidx = -1;

  /* first, build ourselves a list to work with */
  musicqupdate->dispList = nlistAlloc ("temp-musicq-disp", LIST_ORDERED, NULL);

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    mqidx = atoi (p);
    musicqupdate->mqidx = mqidx;
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    /* queue duration */
    musicqupdate->tottime = atol (p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    /* currently playing dbidx (music queue index 0) */
    musicqupdate->currdbidx = atol (p);
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  idx = 1;
  while (p != NULL) {
    musicqupditem = mdmalloc (sizeof (mp_musicqupditem_t));
    assert (musicqupditem != NULL);
    musicqupditem->dispidx = atoi (p);

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupditem->uniqueidx = atol (p);
    }

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupditem->dbidx = atol (p);
    }
    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    if (p != NULL) {
      musicqupditem->pauseind = atoi (p);
    }

    nlistSetData (musicqupdate->dispList, idx, musicqupditem);

    p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
    ++idx;
  }

  return musicqupdate;
}

void
msgparseMusicQueueDataFree (mp_musicqupdate_t *musicqupdate)
{
  if (musicqupdate != NULL) {
    nlistFree (musicqupdate->dispList);
    musicqupdate->dispList = NULL;
    mdfree (musicqupdate);
  }
}

mp_songselect_t *
msgparseSongSelect (char *args)
{
  int               mqidx;
  char              *p;
  char              *tokstr;
  mp_songselect_t   *songselect;


  songselect = mdmalloc (sizeof (mp_songselect_t));
  songselect->mqidx = 0;
  songselect->loc = 0;

  p = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    mqidx = atoi (p);
    songselect->mqidx = mqidx;
  }

  p = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (p != NULL) {
    songselect->loc = atol (p);
  }

  return songselect;
}

void
msgparseSongSelectFree (mp_songselect_t *songselect)
{
  if (songselect != NULL) {
    mdfree (songselect);
  }
}
