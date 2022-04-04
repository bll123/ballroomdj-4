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
#include "dance.h"
#include "datafile.h"
#include "genre.h"
#include "log.h"
#include "orgutil.h"
#include "song.h"
#include "tagdef.h"

typedef struct {
  char            *name;
  orgkey_t        orgkey;
  tagdefkey_t     tagkey;
  dfConvFunc_t    convFunc;
} orglookup_t;

orglookup_t orglookup [ORG_MAX_KEY] = {
  { "--",         ORG_TEXT,       -1,         NULL, },  // not used here
  { "ALBUM",      ORG_ALBUM,      TAG_ALBUM,  NULL, },
  { "ALBUMARTIST", ORG_ALBUMARTIST, TAG_ALBUMARTIST, NULL, },
  { "ARTIST",     ORG_ARTIST,     TAG_ARTIST, NULL, },
  { "COMPOSER",   ORG_COMPOSER,   TAG_COMPOSER, NULL, },
  { "DANCE",      ORG_DANCE,      TAG_DANCE,  danceConvDance },
  { "DISC",       ORG_DISC,       TAG_DISCNUMBER, NULL, },
  { "GENRE",      ORG_GENRE,      TAG_GENRE,  genreConv },
  { "TITLE",      ORG_TITLE,      TAG_TITLE,  NULL, },
  { "TRACKNUM",   ORG_TRACKNUM,   TAG_TRACKNUMBER, NULL, },
  { "TRACKNUM0",  ORG_TRACKNUM0,  TAG_TRACKNUMBER, NULL, },
};

typedef struct {
  int             groupnum;
  orgkey_t        orgkey;
  tagdefkey_t     tagkey;
  dfConvFunc_t    convFunc;
} orginfo_t;

#define ORG_FIRST_GRP 1

org_t *
orgAlloc (char *orgpath)
{
  org_t         *org;
  char          *tvalue;
  char          *p;
  char          *tokstr;
  char          *tokstrB;
  int           grpcount;
  orginfo_t     *orginfo;

  org = malloc (sizeof (org_t));
  assert (org != NULL);

  org->orgparsed = slistAlloc ("orgpath", LIST_UNORDERED, free);

  tvalue = strdup (orgpath);
  grpcount = ORG_FIRST_GRP;
  p = strtok_r (tvalue, "{}", &tokstr);

  while (p != NULL) {
    p = strtok_r (p, "%", &tokstrB);
    while (p != NULL) {
      orginfo = malloc (sizeof (orginfo_t));
      orginfo->groupnum = grpcount;
      orginfo->orgkey = ORG_TEXT;
      orginfo->tagkey = 0;
      orginfo->convFunc = NULL;

      /* just do a brute force search... the parse should only be done once */
      for (orgkey_t i = 0; i < ORG_MAX_KEY; ++i) {
        if (strcmp (orglookup [i].name, p) == 0) {
          orginfo->orgkey = orglookup [i].orgkey;
          orginfo->tagkey = orglookup [i].tagkey;
          orginfo->convFunc = orglookup [i].convFunc;
          break;
        }
      }

      slistSetData (org->orgparsed, p, orginfo);
      p = strtok_r (NULL, "%", &tokstrB);
    }

    ++grpcount;
    p = strtok_r (NULL, "{}", &tokstr);
  }
  free (tvalue);

  return org;
}

void
orgFree (org_t *org)
{
  if (org != NULL) {
    if (org->orgparsed != NULL) {
      slistFree (org->orgparsed);
    }
    free (org);
  }
}

char *
orgMakeSongPath (org_t *org, song_t *song)
{
  slistidx_t      iteridx;
  char            *p;
  char            *tp;
  char            tbuff [MAXPATHLEN];
  char            gbuff [MAXPATHLEN];
  char            tmp [40];
  orginfo_t       *orginfo;
  int             grpnum = ORG_FIRST_GRP;
  bool            grpok = false;
  datafileconv_t  conv;


  *tbuff = '\0';
  *gbuff = '\0';
  slistStartIterator (org->orgparsed, &iteridx);
  while ((p = slistIterateKey (org->orgparsed, &iteridx)) != NULL) {
    orginfo = slistGetData (org->orgparsed, p);
    if (orginfo->groupnum != grpnum) {
      if (grpok) {
        strlcat (tbuff, gbuff, sizeof (tbuff));
      }
      *gbuff = '\0';
      grpok = false;
      grpnum = orginfo->groupnum;
    }

    if (orginfo->orgkey != ORG_TEXT) {
      if (orginfo->orgkey == ORG_TRACKNUM ||
          orginfo->orgkey == ORG_TRACKNUM0 ||
          orginfo->orgkey == ORG_DISC ||
          orginfo->convFunc != NULL) {
        ssize_t   val;

        val = songGetNum (song, orginfo->tagkey);
        if (orginfo->orgkey == ORG_TRACKNUM) {
          snprintf (tmp, sizeof (tmp), "%zd", val);
          p = tmp;
        }
        if (orginfo->orgkey == ORG_DISC) {
          snprintf (tmp, sizeof (tmp), "%02zd", val);
          p = tmp;
        }
        if (orginfo->orgkey == ORG_TRACKNUM0) {
          snprintf (tmp, sizeof (tmp), "%03zd", val);
          p = tmp;
        }
        if (orginfo->convFunc != NULL) {
          conv.valuetype = VALUE_NUM;
          conv.u.num = val;
          orginfo->convFunc (&conv);
          p = conv.u.str;
        }
      } else {
        p = songGetStr (song, orginfo->tagkey);
      }

      if (orginfo->orgkey == ORG_ALBUMARTIST) {
        /* if the albumartist is empty, replace it with the artist */
        if (p == NULL || *p == '\0') {
          p = songGetStr (song, TAG_ARTIST);
        }
      }

      if (orginfo->orgkey == ORG_COMPOSER) {
        /* if the composer is the same as the albumartist or artist */
        /* leave it empty */
        if (p != NULL && *p) {
          tp = songGetStr (song, TAG_ALBUMARTIST);
          if (tp != NULL && strcmp (p, tp) == 0) {
            p = "";
          }
          tp = songGetStr (song, TAG_ARTIST);
          if (tp != NULL && strcmp (p, tp) == 0) {
            p = "";
          }
        }
      }

      if (p != NULL && *p) {
        grpok = true;
      }
    }
    if (p != NULL && *p) {
      strlcat (gbuff, p, sizeof (gbuff));
    }
  }

  if (grpok) {
    strlcat (tbuff, gbuff, sizeof (tbuff));
  }

  p = strdup (tbuff);
  return p;
}
