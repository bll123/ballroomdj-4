#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "log.h"
#include "osutils.h"
#include "pathutil.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"

static void audiotagParseTags (slist_t *tagdata, char *data, int tagtype);
static void audiotagCreateLookupTable (int tagtype);
static void audiotagParseNumberPair (char *data, int *a, int *b);

static slist_t    * tagLookup [TAG_TYPE_MAX] = { NULL, NULL, NULL };

/*
 * .m4a:
 *    trkn=(17, 21)
 * mp3
 *    trck=17/21
 */

char *
audiotagReadTags (char *ffn, long count)
{
  char        tmpfn [MAXPATHLEN];
  char        buff [40];
  char        *data;
  char        * targv [5];

  snprintf (buff, sizeof (buff), "%s-%d", AUDIOTAG_TMP_FILE, count);
  pathbldMakePath (tmpfn, sizeof (tmpfn), "",
      buff, ".txt", PATHBLD_MP_TMPDIR);


  targv [0] = sysvarsGetStr (SV_PYTHON_PATH);
  targv [1] = sysvarsGetStr (SV_PYTHON_MUTAGEN);
  targv [2] = ffn;
  targv [3] = NULL;

  osProcessStart (targv, OS_PROC_WAIT, NULL, tmpfn);
  data = filedataReadAll (tmpfn);
  fileopDelete (tmpfn);
  return data;
}

slist_t *
audiotagParseData (char *ffn, char *data)
{
  slist_t     *tagdata;
  pathinfo_t  *pi;
  int         tagtype;


  tagdata = slistAlloc ("atag", LIST_ORDERED, free);
  pi = pathInfo (ffn);

  tagtype = TAG_TYPE_MP3;
  if (pathInfoExtCheck (pi, ".mp3") ||
      pathInfoExtCheck (pi, ".MP3")) {
    tagtype = TAG_TYPE_MP3;
  }
  if (pathInfoExtCheck (pi, ".m4a") ||
      pathInfoExtCheck (pi, ".M4A")) {
    tagtype = TAG_TYPE_M4A;
  }
  if (pathInfoExtCheck (pi, ".wma") ||
      pathInfoExtCheck (pi, ".WMA")) {
    tagtype = TAG_TYPE_WMA;
  }
  audiotagParseTags (tagdata, data, tagtype);

  pathInfoFree (pi);
  return tagdata;
}

static void
audiotagParseTags (slist_t *tagdata, char *data, int tagtype)
{
  char      *tstr;
  char      *tokstr;
  char      *p;
  char      *tokstrB;
  char      *pC;
  char      *tokstrC;
  char      *tagname;
  char      duration [40];
  char      pbuff [40];
  int       count;
  bool      haveduration;
  bool      rewrite;
  bool      skip;

  audiotagCreateLookupTable (tagtype);

/*
 * mutagen output:
 *
 * -- /home/bll/s/ballroomdj/test.dir/music dir/05 Rumba.mp3
 * - MPEG 1 layer 3, 64000 bps (CBR?), 48000 Hz, 1 chn, 304.54 seconds (audio/mp3)
 * TALB=130.01-alb
 * TIT2=05 Rumba
 * TPE1=130.01-art
 * TPE2=130.01-albart
 * TRCK=122
 * TXXX=DANCE=Rumba
 * TXXX=DANCERATING=Good
 * TXXX=STATUS=Complete
 */

  tstr = strtok_r (data, "\r\n", &tokstr);
  count = 0;
  haveduration = false;
  rewrite = false;
  while (tstr != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "%s", tstr);
    skip = false;
    if (count == 1) {
      p = strstr (tstr, "seconds");
      if (p != NULL) {
        double tm;

        p -= 2;  // should be pointing at last digit of seconds
        while (*p != ' ' && p > tstr) {
          --p;
        }
        ++p;
        tm = atof (p);
        tm *= 1000.0;
        snprintf (duration, sizeof (duration), "%.0f", tm);
        slistSetStr (tagdata, "DURATION", duration);
        haveduration = true;
      } else {
        logMsg (LOG_DBG, LOG_DBUPDATE, "no 'seconds' found");
      }
    }

    if (count > 1) {
      p = strtok_r (tstr, "=", &tokstrB);
      if (p == NULL) {
        tstr = strtok_r (NULL, "\r\n", &tokstr);
        ++count;
        continue;
      }
      if (strcmp (p, "TXXX") == 0) {
        p = strtok_r (NULL, "=", &tokstrB);
      }

      /* p is pointing to the tag name */
      tagname = slistGetStr (tagLookup [tagtype], p);
      logMsg (LOG_DBG, LOG_DBUPDATE, "tag: %s raw-tag: %s", tagname, p);

      if (tagname != NULL && *tagname != '\0') {
        p = strtok_r (NULL, "=", &tokstrB);
        /* p is pointing to the tag data */
        if (p == NULL) {
          p = "";
        }

        if (strcmp (tagname, "DURATION") == 0) {
          if (haveduration) {
            if (strcmp (p, duration) != 0) {
              rewrite = true;
            }
            skip = true;
          }
        }

        /* track number / track total handling */
        if (strcmp (tagname, "TRACKNUMBER") == 0) {
          int   tnum, ttot;
          audiotagParseNumberPair (p, &tnum, &ttot);

          snprintf (pbuff, sizeof (pbuff), "%d", ttot);
          slistSetStr (tagdata, "TRACKTOTAL", pbuff);
          snprintf (pbuff, sizeof (pbuff), "%d", tnum);
          p = pbuff;
        }

        /* disc number / disc total handling */
        if (strcmp (tagname, "DISCNUMBER") == 0) {
          int   dnum, dtot;
          audiotagParseNumberPair (p, &dnum, &dtot);

          snprintf (pbuff, sizeof (pbuff), "%d", dtot);
          slistSetStr (tagdata, "DISCTOTAL", pbuff);
          snprintf (pbuff, sizeof (pbuff), "%d", dnum);
          p = pbuff;
        }

        /* old songend/songstart handling */
        if (strcmp (tagname, "SONGSTART") == 0 ||
            strcmp (tagname, "SONGEND") == 0) {
          char      *tmp;
          double    tm = 0.0;

          tmp = strdup (p);
          pC = strtok_r (tmp, ":", &tokstrC);
          if (pC != NULL) {
            rewrite = true;
            tm += atof (pC) * 60.0;
            pC = strtok_r (NULL, ":", &tokstrC);
            tm += atof (pC);
            tm *= 1000;
            snprintf (pbuff, sizeof (pbuff), "%.0f\n", tm);
            p = pbuff;
          }
          free (tmp);
        }

        /* musicbrainz_trackid handling */
        if (strcmp (tagname, "MUSICBRAINZ_TRACKID") == 0) {
          /* note that MUSICBRAINZ_TRACKID may appear in a TXXX tag */
          /* skip the uri */
          p = strtok_r (NULL, "=", &tokstrB);
          if (p != NULL) {
            if (strncmp (p, "b\"", 2) == 0) {
              rewrite = true;
              p += 2;
            }
            if (strncmp (p, "b'", 2) == 0) {
              rewrite = true;
              p += 2;
            }
            stringTrimChar (p, '\'');
            stringTrimChar (p, '"');
          }
        }

        if (! skip && p != NULL && *p != '\0') {
          slistSetStr (tagdata, tagname, p);
        }
      } /* have a tag name */
    } /* tag processing */

    tstr = strtok_r (NULL, "\r\n", &tokstr);
    ++count;
  }
}

static void
audiotagCreateLookupTable (int tagtype)
{
  char    buff [40];
  slist_t * taglist;

  if (tagLookup [tagtype] != NULL) {
    return;
  }

  snprintf (buff, sizeof (buff), "tag-%d", tagtype);
  tagLookup [tagtype] = slistAlloc (buff, LIST_ORDERED, free);
  taglist = tagLookup [tagtype];

  for (int i = 0; i < MAX_TAG_KEY; ++i) {
    if (tagdefs [i].tag == NULL) {
      continue;
    }

    if (tagdefs [i].audiotags [tagtype] != NULL &&
       strcmp (tagdefs [i].audiotags [tagtype], "TXXX") != 0) {
      slistSetStr (taglist, tagdefs [i].audiotags [tagtype], tagdefs [i].tag);
    }
    /* always add the tag/tag mapping to handle flac, ogg, etc. */
    slistSetStr (taglist, tagdefs [i].tag, tagdefs [i].tag);
  }
}

static void
audiotagParseNumberPair (char *data, int *a, int *b)
{
  char    *p;

  /* apple style track number */
  if (*data == '(') {
    p = data;
    ++p;
    *a = atoi (p);
    p = strstr (p, " ");
    if (p != NULL) {
      ++p;
      *b = atoi (p);
    }
  }
  p = strstr (data, "/");
  if (p != NULL) {
    *a = atoi (data);
    ++p;
    *b = atoi (p);
  }
}
