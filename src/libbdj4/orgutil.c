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
#include "rating.h"
#include "song.h"
#include "status.h"
#include "sysvars.h"
#include "tagdef.h"

typedef struct {
  char            *name;
  tagdefkey_t     tagkey;
  dfConvFunc_t    convFunc;
} orglookup_t;

orglookup_t orglookup [ORG_MAX_KEY] = {
  [ORG_TEXT]        = { "--",           -1,               NULL, }, // not used
  [ORG_ALBUM]       = { "ALBUM",        TAG_ALBUM,        NULL, },
  [ORG_ALBUMARTIST] = { "ALBUMARTIST",  TAG_ALBUMARTIST,  NULL, },
  [ORG_ARTIST]      = { "ARTIST",       TAG_ARTIST,       NULL, },
  [ORG_BPM]         = { "BPM",          TAG_BPM,          NULL, },
  [ORG_BYPASS]      = { "BYPASS",       -1,               NULL, },
  [ORG_COMPOSER]    = { "COMPOSER",     TAG_COMPOSER,     NULL, },
  [ORG_CONDUCTOR]   = { "CONDUCTOR",    TAG_CONDUCTOR,    NULL, },
  [ORG_DANCE]       = { "DANCE",        TAG_DANCE,        danceConvDance },
  [ORG_DISC]        = { "DISC",         TAG_DISCNUMBER,   NULL, },
  [ORG_GENRE]       = { "GENRE",        TAG_GENRE,        genreConv },
  [ORG_RATING]      = { "RATING",       TAG_DANCERATING,  ratingConv },
  [ORG_STATUS]      = { "STATUS",       TAG_STATUS,       statusConv },
  [ORG_TITLE]       = { "TITLE",        TAG_TITLE,        NULL, },
  [ORG_TRACKNUM]    = { "TRACKNUMBER",  TAG_TRACKNUMBER,  NULL, },
  [ORG_TRACKNUM0]   = { "TRACKNUMBER0", TAG_TRACKNUMBER,  NULL, },
};

typedef struct org {
  char          *orgpath;
  char          regexstr [300];
  bdjregex_t    *rx;
  slist_t       *orgparsed;
  char          cachepath [MAXPATHLEN];
  char          **rxdata;
  int           rxlen;
  bool          havealbumartist : 1;
  bool          haveartist : 1;
  bool          havedance : 1;
  bool          havetitle : 1;
} org_t;

typedef struct {
  int             groupnum;
  orgkey_t        orgkey;
  tagdefkey_t     tagkey;
  dfConvFunc_t    convFunc;
  bool            isoptional;
} orginfo_t;

enum {
  ORG_FIRST_GRP = 1,
};

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
  bool          isoptional;


  if (orgpath == NULL) {
    return NULL;
  }

  org = malloc (sizeof (org_t));
  assert (org != NULL);
  org->havealbumartist = false;
  org->haveartist = false;
  org->havedance = false;
  org->havetitle = false;
  org->orgparsed = slistAlloc ("orgpath", LIST_UNORDERED, free);
  /* do not anchor to the beginning -- it may be a full path */
  strlcpy (org->regexstr, "", sizeof (org->regexstr));
  *org->cachepath = '\0';
  org->rxdata = NULL;

  tvalue = strdup (orgpath);
  grpcount = ORG_FIRST_GRP;
  p = strtok_r (tvalue, "{}", &tokstr);

  while (p != NULL) {
    haveorgkey = false;
    isnumeric = false;
    isoptional = false;
    tfirst = "";
    tlast = "";

    p = strtok_r (p, "%", &tokstrB);
    while (p != NULL) {
      orginfo = malloc (sizeof (orginfo_t));
      orginfo->groupnum = grpcount;
      orginfo->orgkey = ORG_TEXT;
      orginfo->tagkey = 0;
      orginfo->convFunc = NULL;
      orginfo->isoptional = false;

      /* just do a brute force search... the parse should only be done once */
      for (orgkey_t i = 0; i < ORG_MAX_KEY; ++i) {
        if (strcmp (orglookup [i].name, p) == 0) {
          orginfo->orgkey = i;
          orginfo->tagkey = orglookup [i].tagkey;
          orginfo->convFunc = orglookup [i].convFunc;
          if (orginfo->orgkey == ORG_ALBUMARTIST) {
            org->havealbumartist = true;
          }
          if (orginfo->orgkey == ORG_ARTIST) {
            org->haveartist = true;
          }
          if (orginfo->orgkey == ORG_DANCE) {
            org->havedance = true;
          }
          if (orginfo->orgkey == ORG_TITLE) {
            org->havetitle = true;
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
        if (grpcount == ORG_FIRST_GRP &&
            (orginfo->orgkey == ORG_DANCE ||
             orginfo->orgkey == ORG_GENRE)) {
          orginfo->isoptional = true;
          isoptional = true;
        }
        /* numerics, always optional */
        if (orginfo->orgkey == ORG_BPM ||
            orginfo->orgkey == ORG_TRACKNUM ||
            orginfo->orgkey == ORG_TRACKNUM0 ||
            orginfo->orgkey == ORG_DISC) {
          orginfo->isoptional = true;
          isoptional = true;
          isnumeric = true;
        }
        haveorgkey = true;
      }

      slistSetData (org->orgparsed, p, orginfo);
      p = strtok_r (NULL, "%", &tokstrB);
    }

    /* attach the regex for this group */
    if (isoptional) {
      strlcat (org->regexstr, "(", sizeof (org->regexstr));
    }
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
        strlcat (org->regexstr, "([^/]+)", sizeof (org->regexstr));
      } else {
        strlcat (org->regexstr, "([^/]+)", sizeof (org->regexstr));
      }
    }
    if (*tlast) {
      char  *tmp;
      tmp = regexEscape (tlast);
      strlcat (org->regexstr, tmp, sizeof (org->regexstr));
      free (tmp);
    }
    if (isoptional) {
      /* optional group */
      strlcat (org->regexstr, ")?", sizeof (org->regexstr));
    }

    ++grpcount;
    p = strtok_r (NULL, "{}", &tokstr);
  }
  strlcat (org->regexstr, "\\.[a-zA-Z0-9]+$", sizeof (org->regexstr));
//fprintf (stderr, "path: %s\n", orgpath);
//fprintf (stderr, "regexstr: %s\n", org->regexstr);
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
      regexGetFree (org->rxdata);
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
  int         inc;

  if (org == NULL) {
    return NULL;
  }

  if (strcmp (org->cachepath, path) != 0) {
    int   c = 0;

    if (org->rxdata != NULL) {
      regexGetFree (org->rxdata);
    }
    strlcpy (org->cachepath, path, sizeof (org->cachepath));
    org->rxdata = regexGet (org->rx, path);
    org->rxlen = 0;
    while (org->rxdata [c] != NULL) {
//fprintf (stderr, "%d %s\n", c, org->rxdata [c]);
      ++c;
    }
    org->rxlen = c;
  }

  if (org->rxdata == NULL) {
    return NULL;
  }

  slistStartIterator (org->orgparsed, &iteridx);
  inc = 0;
  while ((orginfo = slistIterateValueData (org->orgparsed, &iteridx)) != NULL) {
    if (orginfo->isoptional) {
      ++inc;
    }

    if (orginfo->orgkey != ORG_TEXT && orginfo->tagkey == tagkey) {
      int   idx;
      char  *val;

      idx = orginfo->groupnum + inc;
      if (idx >= org->rxlen) {
        return NULL;
      }

      val = org->rxdata [idx];
      if (tagkey == TAG_DISCNUMBER ||
          tagkey == TAG_TRACKNUMBER) {
        if (val == NULL || ! *val) {
          val = "1";
        }
      }
      return val;
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
          conv.num = val;
          orginfo->convFunc (&conv);
          p = conv.str;
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

inline bool
orgHaveDance (org_t *org)
{
  if (org == NULL) {
    return false;
  }
  return org->havedance;
}

void
orgStartIterator (org_t *org, slistidx_t *iteridx)
{
  slistStartIterator (org->orgparsed, iteridx);
}

int
orgIterateTagKey (org_t *org, slistidx_t *iteridx)
{
  orginfo_t   *orginfo;

  while ((orginfo = slistIterateValueData (org->orgparsed, iteridx)) != NULL) {
    if (orginfo->orgkey != ORG_TEXT) {
      return orginfo->tagkey;
    }
  }

  return -1;
}

int
orgIterateOrgKey (org_t *org, slistidx_t *iteridx)
{
  orginfo_t   *orginfo;

  while ((orginfo = slistIterateValueData (org->orgparsed, iteridx)) != NULL) {
    return orginfo->orgkey;
  }

  return -1;
}

int
orgGetTagKey (int orgkey)
{
  if (orgkey < 0) {
    return -1;
  }
  return orglookup [orgkey].tagkey;
}

char *
orgGetText (org_t *org, slistidx_t idx)
{
  orginfo_t *orginfo;

  orginfo = slistGetDataByIdx (org->orgparsed, idx);
  if (orginfo == NULL) {
    return NULL;
  }
  if (orginfo->orgkey != ORG_TEXT) {
    return NULL;
  }
  return slistGetKeyByIdx (org->orgparsed, idx);
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
