#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjstring.h"
#include "log.h"
#include "orgutil.h"
#include "song.h"
#include "tagdef.h"

typedef struct {
  char        *name;
  orgkey_t    orgkey;
  tagdefkey_t tagkey;
} orglookup_t;

orglookup_t orglookup [ORG_MAX_KEY] = {
  { "--",         ORG_TEXT,       -1, },  // not used here
  { "ALBUM",      ORG_ALBUM,      TAG_ALBUM, },
  { "ALBUMARTIST", ORG_ALBUMARTIST, TAG_ALBUMARTIST, },
  { "ARTIST",     ORG_ARTIST,     TAG_ARTIST, },
  { "COMPOSER",   ORG_COMPOSER,   TAG_COMPOSER, },
  { "DANCE",      ORG_DANCE,      TAG_DANCE, },
  { "DISC",       ORG_DISC,       TAG_DISCNUMBER, },
  { "GENRE",      ORG_GENRE,      TAG_GENRE, },
  { "TITLE",      ORG_TITLE,      TAG_TITLE, },
  { "TRACKNUM",   ORG_TRACKNUM,   TAG_TRACKNUMBER, },
  { "TRACKNUM0",  ORG_TRACKNUM0,  TAG_TRACKNUMBER, },
};

org_t *
orgAlloc (char *orgpath)
{
  org_t         *org;
  char          *tvalue;
  char          *p;
  char          *tokstr;
  char          *tokstrB;

  org = malloc (sizeof (org_t));
  assert (org != NULL);

  org->orgparsed = slistAlloc ("orgpath", LIST_UNORDERED, NULL);

  tvalue = strdup (orgpath);
  p = strtok_r (tvalue, "{}", &tokstr);

  while (p != NULL) {
    p = strtok_r (p, "%", &tokstrB);
    while (p != NULL) {
      nlistidx_t  val;

      val = ORG_TEXT;
      for (int i = 0; i < ORG_MAX_KEY; ++i) {
        if (strcmp (orglookup [i].name, p) == 0) {
          val = orglookup [i].orgkey;
        }
      }
      slistSetNum (org->orgparsed, p, val);
      p = strtok_r (NULL, "%", &tokstrB);
    }

    p = strtok_r (NULL, "{}", &tokstr);
  }

  return org;
}

void
orgFree (org_t *org)
{
  if (org != NULL) {
    free (org);
  }
}

char *
orgMakeSongPath (org_t *org, song_t *song)
{
  slistidx_t      iteridx;
  char            *p;
  char            tbuff [MAXPATHLEN];
  nlistidx_t      orgkey;
  nlistidx_t      tagkey;

  *tbuff = '\0';
  slistStartIterator (org->orgparsed, &iteridx);
  while ((p = slistIterateKey (org->orgparsed, &iteridx)) != NULL) {
    orgkey = slistGetNum (org->orgparsed, p);
    tagkey = orglookup [orgkey].tagkey;
    if (orgkey != ORG_TEXT) {
      p = songGetStr (song, tagkey);
    }
    strlcat (tbuff, p, sizeof (tbuff));
  }

  p = strdup (tbuff);
  return p;
}
