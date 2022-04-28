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
#include <wchar.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "dance.h"
#include "datafile.h"
#include "genre.h"
#include "log.h"
#include "orgutil.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"

typedef enum {
  ORG_TEXT,
  ORG_ALBUM,
  ORG_ALBUMARTIST,
  ORG_ARTIST,
  ORG_COMPOSER,
  ORG_CONDUCTOR,
  ORG_DANCE,
  ORG_DISC,
  ORG_GENRE,
  ORG_TITLE,
  ORG_TRACKNUM,
  ORG_TRACKNUM0,
  ORG_MAX_KEY,
} orgkey_t;

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
  { "CONDUCTOR",  ORG_CONDUCTOR,  TAG_CONDUCTOR, NULL, },
  { "DANCE",      ORG_DANCE,      TAG_DANCE,  danceConvDance },
  { "DISC",       ORG_DISC,       TAG_DISCNUMBER, NULL, },
  { "GENRE",      ORG_GENRE,      TAG_GENRE,  genreConv },
  { "TITLE",      ORG_TITLE,      TAG_TITLE,  NULL, },
  { "TRACKNUMBER",ORG_TRACKNUM,   TAG_TRACKNUMBER, NULL, },
  { "TRACKNUMBER0",ORG_TRACKNUM0,  TAG_TRACKNUMBER, NULL, },
};

typedef struct org {
  char          *orgpath;
  char          regexstr [200];
  bdjregex_t    *rx;
  slist_t       *orgparsed;
  const char    *cachepath;
  char          **rxdata;
  int           rxlen;
  bool          havealbumartist : 1;
  bool          haveartist : 1;
} org_t;

typedef struct {
  int             groupnum;
  orgkey_t        orgkey;
  tagdefkey_t     tagkey;
  dfConvFunc_t    convFunc;
} orginfo_t;

#define ORG_FIRST_GRP 1

static void   orgutilClean (char *target, const char *from, size_t sz);

org_t *
orgAlloc (char *orgpath)
{
  org_t         *org;
  char          *tvalue;
  char          *p;
  char          *tfirst;
  char          *tlast;
  char          *tokstr;
  char          *tokstrB;
  int           grpcount;
  orginfo_t     *orginfo;
  bool          haveorgkey;
  bool          isnumeric;


  org = malloc (sizeof (org_t));
  assert (org != NULL);
  org->havealbumartist = false;
  org->haveartist = false;
  org->orgparsed = slistAlloc ("orgpath", LIST_UNORDERED, free);
  strlcpy (org->regexstr, "^", sizeof (org->regexstr));
  org->cachepath = NULL;
  org->rxdata = NULL;

  tvalue = strdup (orgpath);
  grpcount = ORG_FIRST_GRP;
  p = strtok_r (tvalue, "{}", &tokstr);

  while (p != NULL) {
    haveorgkey = false;
    isnumeric = false;
    tfirst = "";
    tlast = "";

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
          if (orginfo->orgkey == ORG_ALBUMARTIST) {
            org->havealbumartist = true;
          }
          if (orginfo->orgkey == ORG_ARTIST) {
            org->haveartist = true;
          }
          break;
        }
      }

      if (orginfo->orgkey == ORG_TEXT) {
        if (haveorgkey) {
          tlast = p;
        } else {
          tfirst = p;
        }
      } else {
        /* numerics */
        if (orginfo->orgkey == ORG_TRACKNUM ||
            orginfo->orgkey == ORG_TRACKNUM0 ||
            orginfo->orgkey == ORG_DISC) {
          isnumeric = true;
        }
        haveorgkey = true;
      }

      slistSetData (org->orgparsed, p, orginfo);
      p = strtok_r (NULL, "%", &tokstrB);
    }

    /* attach the regex for this group */
    if (*tfirst) {
      char *tmp;
      tmp = regexEscape (tfirst);
      strlcat (org->regexstr, tmp, sizeof (org->regexstr));
      free (tmp);
    }
    if (isnumeric) {
      strlcat (org->regexstr, "([0-9]+)", sizeof (org->regexstr));
    } else {
      size_t    len;

      len = strlen (tlast);
      if (len > 0 && tlast [len-1] == '/') {
        strlcat (org->regexstr, "([^/]*)", sizeof (org->regexstr));
      } else {
        strlcat (org->regexstr, "(.*?)", sizeof (org->regexstr));
      }
    }
    if (*tlast) {
      char  *tmp;
      tmp = regexEscape (tlast);
      strlcat (org->regexstr, tmp, sizeof (org->regexstr));
      free (tmp);
    }

    ++grpcount;
    p = strtok_r (NULL, "{}", &tokstr);
  }
  strlcat (org->regexstr, "\\.[a-zA-Z0-9]+$", sizeof (org->regexstr));
  org->rx = regexInit (org->regexstr);
  free (tvalue);

  return org;
}

void
orgFree (org_t *org)
{
  if (org != NULL) {
    if (org->rx != NULL) {
      regexFree (org->rx);
    }
    if (org->rxdata != NULL) {
      free (org->rxdata);
    }
    if (org->orgparsed != NULL) {
      slistFree (org->orgparsed);
    }
    free (org);
  }
}

slist_t *
orgGetList (org_t *org)
{
  if (org == NULL) {
    return NULL;
  }
  return org->orgparsed;
}

char *
orgGetFromPath (org_t *org, const char *path, tagdefkey_t tagkey)
{
  slistidx_t  iteridx;
  orginfo_t   *orginfo;

  if (org == NULL) {
    return NULL;
  }

  if (org->cachepath == NULL || strcmp (org->cachepath, path) != 0) {
    int   c = 0;

    if (org->rxdata != NULL) {
      free (org->rxdata);
    }
    org->cachepath = path;
    org->rxdata = regexGet (org->rx, path);
    org->rxlen = 0;
    while (org->rxdata [c] != NULL) {
      ++c;
    }
    org->rxlen = c;
  }

  if (org->rxdata == NULL) {
    return NULL;
  }

  slistStartIterator (org->orgparsed, &iteridx);
  while ((orginfo = slistIterateValueData (org->orgparsed, &iteridx)) != NULL) {
    if (orginfo->tagkey == tagkey) {
      if (orginfo->groupnum >= org->rxlen) {
        return NULL;
      }
      return org->rxdata [orginfo->groupnum];
    }
  }

  return NULL;
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
  bool            doclean = false;
  datafileconv_t  conv;


  *tbuff = '\0';
  *gbuff = '\0';
  slistStartIterator (org->orgparsed, &iteridx);
  while ((p = slistIterateKey (org->orgparsed, &iteridx)) != NULL) {
    doclean = false;

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
          conv.allocated = false;
          conv.valuetype = VALUE_NUM;
          conv.u.num = val;
          orginfo->convFunc (&conv);
          p = conv.u.str;
        }
      } else {
        p = songGetStr (song, orginfo->tagkey);
        doclean = true;
      }

      /* rule:  if both albumartist and artist are present and the */
      /*        albumartist is the same as the artist, clear the artist. */
      if (orginfo->orgkey == ORG_ARTIST &&
          org->havealbumartist &&
          org->haveartist) {
        tp = songGetStr (song, TAG_ALBUMARTIST);
        if (tp != NULL && strcmp (p, tp) == 0) {
          p = "";
        }
      }

      /* rule:  if the albumartist is empty replace it with the artist */
      if (orginfo->orgkey == ORG_ALBUMARTIST) {
        /* if the albumartist is empty, replace it with the artist */
        if (p == NULL || *p == '\0') {
          p = songGetStr (song, TAG_ARTIST);
          doclean = true;
        }
      }

      /* rule:  only use the composer/conductor groups if the genre is classical */
      if (orginfo->orgkey == ORG_COMPOSER ||
          orginfo->orgkey == ORG_CONDUCTOR) {
        ilistidx_t  genreidx;
        int         clflag;
        genre_t     *genres;

        genreidx = songGetNum (song, TAG_GENRE);
        genres = bdjvarsdfGet (BDJVDF_GENRES);
        clflag = genreGetClassicalFlag (genres, genreidx);
        if (! clflag) {
          p = "";
        }
      }

      /* rule:  the composer group is cleared if the computer matches the */
      /*        albumartist or the artist. */
      if (orginfo->orgkey == ORG_COMPOSER) {
        /* if the composer is the same as the albumartist or artist */
        /* leave it empty */
        if (p != NULL && *p) {
          tp = songGetStr (song, TAG_ALBUMARTIST);
          if (tp != NULL && strcmp (p, tp) == 0) {
            p = "";
            doclean = false;
          }
          tp = songGetStr (song, TAG_ARTIST);
          if (tp != NULL && strcmp (p, tp) == 0) {
            p = "";
            doclean = false;
          }
        }
      }

      if (p != NULL && *p) {
        grpok = true;
      }
    }
    if (p != NULL && *p) {
      char  sbuff [MAXPATHLEN];

      strlcpy (sbuff, p, sizeof (sbuff));
      if (doclean) {
        orgutilClean (sbuff, p, sizeof (sbuff));
      }
      strlcat (gbuff, sbuff, sizeof (gbuff));
    }
  }

  if (grpok) {
    strlcat (tbuff, gbuff, sizeof (tbuff));
  }

  p = strdup (tbuff);
  return p;
}

/* internal routines */

/* this must be locale aware, otherwise the characters that are */
/* being cleaned might appear within a multi-byte sequence */
static void
orgutilClean (char *target, const char *from, size_t sz)
{
  size_t      bytelen;
  size_t      slen;
  size_t      mlen;
  mbstate_t   ps;
  const char  *tstr;
  char        *tgtp;
  size_t      tgtlen;

  tgtp = target;
  tgtlen = 0;

  memset (&ps, 0, sizeof (mbstate_t));
  bytelen = strlen (from);
  slen = bytelen;
  tstr = from;

  while (slen > 0) {
    mlen = mbrlen (tstr, slen, &ps);
    if (mlen <= 0) {
      /* bad character; do not copy to target */
      tstr += 1;
      slen -= 1;
      continue;
    }

    if (mlen == 1) {
      bool skip = false;

      /* period at the end of the string is not valid on windows */
      /* this does not handle multiple . at the end of the string */
      if (*tstr == '.' && slen == 1) {
        skip = true;
      }

      /* always skip / and \ characters */
      if (! skip && (*tstr == '/' || *tstr == '\\')) {
        skip = true;
      }

      /*      mp3tag: * :             | < >     " */
      /*     windows: * : ( ) &     ^ | < > ? ' " */
      /* linux/macos: *       & [ ]   | < > ? ' " */

      /* these characters just cause issues with filename handling */
      /* using scripts */
      if (! skip &&
          (*tstr == '|' || *tstr == '<' || *tstr == '>' || *tstr == '?')) {
        skip = true;
      }
      /* more issues with filename handling */
      if (! skip &&
          (*tstr == '*' || *tstr == '\'' || *tstr == '"' || *tstr == '&')) {
        skip = true;
      }
      /* windows special characters */
      if (! skip && isWindows ()) {
        if (*tstr == ':' || *tstr == '(' || *tstr == ')' || *tstr == '^') {
          skip = true;
        }
      }
      /* linux/macos special characters */
      if (! skip && ! isWindows ()) {
        if (*tstr == '[' || *tstr == ']') {
          skip = true;
        }
      }
      if (! skip && bdjoptGetNum (OPT_G_AO_REMOVE_SPACE)) {
        if (*tstr == ' ') {
          skip = true;
        }
      }

      if (skip) {
        tstr += mlen;
        slen -= mlen;
        continue;
      }
    }

    if (tgtlen + mlen + 1 >= sz) {
      break;
    }

    memcpy (tgtp, tstr, mlen);
    tgtp += mlen;
    tgtlen += mlen;
    tstr += mlen;
    slen -= mlen;
  }

  *tgtp = '\0';
}
