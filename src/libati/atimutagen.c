/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "ati.h"
#include "audiofile.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "sysvars.h"
#include "tagdef.h"

typedef struct atidata {
  const char    *python;
  const char    *mutagen;
  int           writetags;
  taglookup_t   tagLookup;
  tagcheck_t    tagCheck;
} atidata_t;

static void atimutagenParseNumberPair (atidata_t *atidata, char *data, int *a, int *b);

atidata_t *
atiiInit (const char *atipkg,
    int writetags, taglookup_t tagLookup, tagcheck_t tagCheck)
{
  atidata_t *atidata;

  atidata = mdmalloc (sizeof (atidata_t));
  atidata->python = sysvarsGetStr (SV_PATH_PYTHON);
  atidata->mutagen = sysvarsGetStr (SV_PYTHON_MUTAGEN);
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char        * data;
  const char  * targv [5];
  size_t      retsz;
  int         rc;

  targv [0] = atidata->python;
  targv [1] = atidata->mutagen;
  targv [2] = ffn;
  targv [3] = NULL;

  data = mdmalloc (ATI_TAG_BUFF_SIZE);
  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, ATI_TAG_BUFF_SIZE, &retsz);
  for (size_t i = 0; i < retsz; ++i) {
    if (data [i] == '\0') {
      data [i] = 'X';
    }
  }
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data,
    int tagtype, int *rewrite)
{
  char        *tstr;
  char        *tokstr;
  char        *p;
  char        *tokstrB;
  char        *pC;
  const char  *tagname;
  char        duration [40];
  char        pbuff [40];
  char        tbuff [1024];
  int         count;
  int         tagkey;
  int         writetags;

  writetags = atidata->writetags;

  /*
   * mutagen output:
   *
   * m4a output from mutagen is very bizarre for freeform tags
   * - MPEG-4 audio (ALAC), 210.81 seconds, 1536000 bps (audio/mp4)
   * ----:BDJ4:DANCE=MP4FreeForm(b'Waltz', <AtomDataType.UTF8: 1>)
   * ----:com.apple.iTunes:MusicBrainz Track Id=MP4FreeForm(b'blah', <AtomDataType.UTF8: 1>)
   * ----:BDJ4:NOTES=MP4FreeForm(b'NOTES3 NOTES4 \xc3\x84\xc3\x84\xc3\x84\xc3\x84\xc3\x84\xc3\x84\xc3\x84\xc3\x84', <AtomDataType.UTF8: 1>)
   * trkn=(1, 0)
   * ©ART=2NE1
   * ©alb=2nd Mini Album
   * ©day=2011
   * ©gen=Electronic
   * ©nam=xyzzy
   * ©too=Lavf56.15.102
   *
   * - FLAC, 20.80 seconds, 44100 Hz (audio/flac)
   * ARTIST=artist
   * TITLE=zzz
   *
   * - MPEG 1 layer 3, 64000 bps (CBR?), 48000 Hz, 1 chn, 304.54 seconds (audio/mp3)
   * TALB=130.01-alb
   * TIT2=05 Rumba
   * TPE1=130.01-art
   * TPE2=130.01-albart
   * TRCK=122
   * TXXX=DANCE=Rumba
   * TXXX=DANCERATING=Good
   * TXXX=STATUS=Complete
   * UFID=http://musicbrainz.org=...
   */

  tstr = strtok_r (data, "\r\n", &tokstr);
  count = 0;
  while (tstr != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "raw: %s", tstr);
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
        slistSetStr (tagdata, tagdefs [TAG_DURATION].tag, duration);
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

      if (strcmp (p, "TXXX") == 0 || strcmp (p, "UFID") == 0) {
        /* find the next = */
        strtok_r (NULL, "=", &tokstrB);
        /* replace the = after the TXXX and use the full TXXX=tag to search */
        *(p+strlen(p)) = '=';

        /* handle TXXX=MUSICBRAINZ_TRACKID (should be UFID) */
        if (strcmp (p, "TXXX=MUSICBRAINZ_TRACKID") == 0) {
          p = "UFID=http://musicbrainz.org";
        }
      }

      /* p is pointing to the tag name */

      if (strcmp (p, "TXXX=VARIOUSARTISTS") == 0 ||
          strcmp (p, "VARIOUSARTISTS") == 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE, "rewrite: various");
        *rewrite |= AF_REWRITE_VARIOUS;
      }

      tagname = atidata->tagLookup (tagtype, p);
      if (tagname != NULL) {
        logMsg (LOG_DBG, LOG_DBUPDATE, "taglookup: %s %s", p, tagname);
        tagkey = atidata->tagCheck (writetags, tagtype, tagname, AF_REWRITE_NONE);
        logMsg (LOG_DBG, LOG_DBUPDATE, "tag: %s raw-tag: %s", tagname, p);
      }

      if (tagname != NULL && *tagname != '\0') {
        int   outidx;

        p = strtok_r (NULL, "=", &tokstrB);
        /* p is pointing to the tag data */
        if (p == NULL) {
          p = "";
        }

        /* mutagen-inspect dumps out python code */
        if (strstr (p, "MP4FreeForm(") != NULL) {
          p = strstr (p, "(");
          ++p;
          if (strncmp (p, "b'", 2) == 0) {
            p += 2;
            pC = strstr (p, "'");
            if (pC != NULL) {
              *pC = '\0';
            }
          }
          if (strncmp (p, "b\"", 2) == 0) {
            p += 2;
            pC = strstr (p, "\"");
            if (pC != NULL) {
              *pC = '\0';
            }
          }

          /* now convert all of the \xNN values */
          *tbuff = '\0';
          pC = p;
          outidx = 0;
          while (*pC && outidx < (int) sizeof (tbuff)) {
            if (strncmp (pC, "\\x", 2) == 0) {
              int     rc;

              rc = sscanf (pC, "\\x%hhx", &tbuff [outidx]);
              if (rc == 1) {
                ++outidx;
                pC += 4;
              } else {
                tbuff [outidx] = *pC;
                ++outidx;
                ++pC;
              }
            } else {
              tbuff [outidx] = *pC;
              ++outidx;
              ++pC;
            }
            tbuff [outidx] = '\0';
          }
          p = tbuff;
        }

        /* put in some extra checks for odd mutagen python output */
        /* this stuff always appears in the UFID tag output */
        if (p != NULL) {
          if (tagkey == TAG_RECORDING_ID) {
            /* check for old mangled data */
            /* note that mutagen-inspect always outputs the b', */
            /* so we need to look for b'', b'b', etc. */
            if (strncmp (p, "b''", 3) == 0 ||
                strncmp (p, "b'b'", 4) == 0) {
              *rewrite |= AF_REWRITE_MB;
            }
          }
          /* the while loops are to handle old messed-up data */
          while (strncmp (p, "b'", 2) == 0) {
            p += 2;
            while (*p == '\'') {
              ++p;
            }
            stringTrimChar (p, '\'');
          }
        }

        /* track number / track total handling */
        if (strcmp (tagname, tagdefs [TAG_TRACKNUMBER].tag) == 0) {
          int   tnum, ttot;
          atimutagenParseNumberPair (atidata, p, &tnum, &ttot);

          if (ttot != 0) {
            snprintf (pbuff, sizeof (pbuff), "%d", ttot);
            slistSetStr (tagdata, tagdefs [TAG_TRACKTOTAL].tag, pbuff);
          }
          snprintf (pbuff, sizeof (pbuff), "%d", tnum);
          p = pbuff;
        }

        /* disc number / disc total handling */
        if (strcmp (tagname, tagdefs [TAG_DISCNUMBER].tag) == 0) {
          int   dnum, dtot;
          atimutagenParseNumberPair (atidata, p, &dnum, &dtot);

          if (dtot != 0) {
            snprintf (pbuff, sizeof (pbuff), "%d", dtot);
            slistSetStr (tagdata, tagdefs [TAG_DISCTOTAL].tag, pbuff);
          }
          snprintf (pbuff, sizeof (pbuff), "%d", dnum);
          p = pbuff;
        }

        /* p is pointing to the tag value */

        if (p != NULL && *p != '\0') {
          slistSetStr (tagdata, tagname, p);
        }
      } /* have a tag name */
    } /* tag processing */

    tstr = strtok_r (NULL, "\r\n", &tokstr);
    ++count;
  }
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int filetype, int writetags)
{
  return 0;
}

static void
atimutagenParseNumberPair (atidata_t *atidata, char *data, int *a, int *b)
{
  char    *p;

  *a = 0;
  *b = 0;

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
    return;
  }

  /* track/total style */
  p = strstr (data, "/");
  *a = atoi (data);
  if (p != NULL) {
    ++p;
    *b = atoi (p);
  }
}


