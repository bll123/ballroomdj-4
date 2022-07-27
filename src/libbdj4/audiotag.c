#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdjopt.h"
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
#include "tmutil.h"

enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MPEG4,
  AFILE_TYPE_MP3,
  AFILE_TYPE_OGGOPUS,
  AFILE_TYPE_OGGVORBIS,
  AFILE_TYPE_WMA,
};

static void audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype);
static void audiotagParseTags (slist_t *tagdata, char *data, int tagtype);
static void audiotagCreateLookupTable (int tagtype);
static void audiotagParseNumberPair (char *data, int *a, int *b);
static void audiotagWriteMP3Tags (const char *ffn, slist_t *tagdata, int writetags);
static void audiotagWriteOtherTags (const char *ffn, slist_t *tagdata, int tagtype, int filetype, int writetags);
static void audiotagPreparePair (slist_t *tagdata, char *buff, size_t sz,
    tagdefkey_t taga, tagdefkey_t tagb, int tagtype);
static bool audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value);
static void audiotagRunUpdate (const char *fn);
static void audiotagMakeTempFilename (char *fn, size_t sz);

static slist_t    * tagLookup [TAG_TYPE_MAX];

static ssize_t globalCounter = 0;

void
audiotagInit (void)
{
  for (int i = 0; i < TAG_TYPE_MAX; ++i) {
    tagLookup [i] = NULL;
  }
}

void
audiotagCleanup (void)
{
  for (int i = 0; i < TAG_TYPE_MAX; ++i) {
    if (tagLookup [i] != NULL) {
      slistFree (tagLookup [i]);
      tagLookup [i] = NULL;
    }
  }
}

/*
 * .m4a:
 *    trkn=(17, 21)
 * mp3
 *    trck=17/21
 */

char *
audiotagReadTags (const char *ffn)
{
  char        * data;
  const char  * targv [5];

  targv [0] = sysvarsGetStr (SV_PATH_PYTHON);
  targv [1] = sysvarsGetStr (SV_PYTHON_MUTAGEN);
  targv [2] = ffn;
  targv [3] = NULL;

  data = malloc (AUDIOTAG_TAG_BUFF_SIZE);
  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, AUDIOTAG_TAG_BUFF_SIZE);
  return data;
}

slist_t *
audiotagParseData (const char *ffn, char *data)
{
  slist_t     *tagdata;
  int         tagtype;
  int         filetype;

  tagdata = slistAlloc ("atag", LIST_ORDERED, free);
  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  audiotagParseTags (tagdata, data, tagtype);
  return tagdata;
}

void
audiotagWriteTags (const char *ffn, slist_t *tagdata)
{
  int         tagtype;
  int         filetype;
  int         writetags;

  logProcBegin (LOG_PROC, "audiotagsWriteTags");

  writetags = bdjoptGetNum (OPT_G_WRITETAGS);
  if (writetags == WRITE_TAGS_NONE) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "write-none");
    return;
  }

  audiotagDetermineTagType (ffn, &tagtype, &filetype);

  if (tagtype != TAG_TYPE_VORBIS &&
      tagtype != TAG_TYPE_MP3 &&
      tagtype != TAG_TYPE_MPEG4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-tag");
    return;
  }

  if (filetype != AFILE_TYPE_OGGOPUS &&
      filetype != AFILE_TYPE_OGGVORBIS &&
      filetype != AFILE_TYPE_FLAC &&
      filetype != AFILE_TYPE_MP3 &&
      filetype != AFILE_TYPE_MPEG4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-file");
    return;
  }

  if (tagtype == TAG_TYPE_MP3 && filetype == AFILE_TYPE_MP3) {
    audiotagWriteMP3Tags (ffn, tagdata, writetags);
  } else {
    audiotagWriteOtherTags (ffn, tagdata, tagtype, filetype, writetags);
  }
}

static void
audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype)
{
  pathinfo_t  *pi;

  pi = pathInfo (ffn);

  *filetype = AFILE_TYPE_UNKNOWN;
  *tagtype = TAG_TYPE_VORBIS;
  if (pathInfoExtCheck (pi, ".mp3") ||
      pathInfoExtCheck (pi, ".MP3")) {
    *tagtype = TAG_TYPE_MP3;
    *filetype = AFILE_TYPE_MP3;
  }
  if (pathInfoExtCheck (pi, ".m4a") ||
      pathInfoExtCheck (pi, ".M4A")) {
    *tagtype = TAG_TYPE_MPEG4;
    *filetype = AFILE_TYPE_MPEG4;
  }
  if (pathInfoExtCheck (pi, ".wma") ||
      pathInfoExtCheck (pi, ".WMA")) {
    *tagtype = TAG_TYPE_WMA;
    *filetype = AFILE_TYPE_WMA;
  }
  if (pathInfoExtCheck (pi, ".ogg") ||
      pathInfoExtCheck (pi, ".OGG")) {
    *filetype = AFILE_TYPE_OGGVORBIS;
  }
  if (pathInfoExtCheck (pi, ".opus") ||
      pathInfoExtCheck (pi, ".OPUS")) {
    *filetype = AFILE_TYPE_OGGOPUS;
  }
  if (pathInfoExtCheck (pi, ".flac") ||
      pathInfoExtCheck (pi, ".FLAC")) {
    *filetype = AFILE_TYPE_FLAC;
  }

  pathInfoFree (pi);
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
 * mp4 output is very bizarre for freeform tags
 * - MPEG-4 audio (ALAC), 210.81 seconds, 1536000 bps (audio/mp4)
 * ----:BDJ4:DANCE=MP4FreeForm(b'Waltz', <AtomDataType.UTF8: 1>)
 * ----:com.apple.iTunes:MusicBrainz Track Id=MP4FreeForm(b'blah', <AtomDataType.UTF8: 1>)
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
  haveduration = false;
  rewrite = false;
  while (tstr != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "raw: %s", tstr);
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
        slistSetStr (tagdata, tagdefs [TAG_DURATION].tag, duration);
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

      if (strcmp (p, "TXXX") == 0 || strcmp (p, "UFID") == 0) {
        /* find the next = */
        strtok_r (NULL, "=", &tokstrB);
        /* replace the = after the TXXX and use the full TXXX=tag to search */
        *(p+strlen(p)) = '=';

        /* handle TXXX=MUSICBRAINZ_TRACKID (should be UFID) */
        if (strcmp (p, "TXXX=MUSICBRAINZ_TRACKID") == 0) {
          rewrite = true;
          p = "UFID=http://musicbrainz.org";
        }
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
        }

        /* put in some extra checks for odd mutagen python output */
        /* this stuff always appears in the UFID tag output */
        if (p != NULL) {
          if (strncmp (p, "b\"", 2) == 0) {
            p += 2;
            stringTrimChar (p, '"');
          }
          if (strncmp (p, "b'", 2) == 0) {
            p += 2;
            stringTrimChar (p, '\'');
          }
        }

        if (strcmp (tagname, tagdefs [TAG_DURATION].tag) == 0) {
          if (haveduration) {
            if (strcmp (p, duration) != 0) {
              rewrite = true;
            }
            skip = true;
          }
        }

        /* track number / track total handling */
        if (strcmp (tagname, tagdefs [TAG_TRACKNUMBER].tag) == 0) {
          int   tnum, ttot;
          audiotagParseNumberPair (p, &tnum, &ttot);

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
          audiotagParseNumberPair (p, &dnum, &dtot);

          if (dtot != 0) {
            snprintf (pbuff, sizeof (pbuff), "%d", dtot);
            slistSetStr (tagdata, tagdefs [TAG_DISCTOTAL].tag, pbuff);
          }
          snprintf (pbuff, sizeof (pbuff), "%d", dnum);
          p = pbuff;
        }

        /* old songend/songstart handling */
        if (strcmp (tagname, tagdefs [TAG_SONGSTART].tag) == 0 ||
            strcmp (tagname, tagdefs [TAG_SONGEND].tag) == 0) {
          char      *tmp;
          double    tm = 0.0;

          if (strstr (p, ":") != NULL) {
            tmp = strdup (p);
            pC = strtok_r (tmp, ":", &tokstrC);
            if (pC != NULL) {
              tm += atof (pC) * 60.0;
              pC = strtok_r (NULL, ":", &tokstrC);
              tm += atof (pC);
              tm *= 1000;
              snprintf (pbuff, sizeof (pbuff), "%.0f", tm);
              p = pbuff;
            }
            free (tmp);
          }
        }

        /* old volumeadjustperc handling */
        if (strcmp (tagname, tagdefs [TAG_VOLUMEADJUSTPERC].tag) == 0) {
          double    tm = 0.0;

          pC = strstr (p, ".");
          if (pC == NULL) {
            tm += atof (p);
            tm /= 10;
            snprintf (pbuff, sizeof (pbuff), "%.1f", tm);
            p = pbuff;
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

/*
 * for looking up the audio file tag, converting to the internal tag name
 */
static void
audiotagCreateLookupTable (int tagtype)
{
  char    buff [40];
  slist_t * taglist;

  tagdefInit ();

  if (tagLookup [tagtype] != NULL) {
    return;
  }

  snprintf (buff, sizeof (buff), "tag-%d", tagtype);
  tagLookup [tagtype] = slistAlloc (buff, LIST_ORDERED, free);
  taglist = tagLookup [tagtype];

  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (! tagdefs [i].isNormTag && ! tagdefs [i].isBDJTag) {
      continue;
    }
    if (tagdefs [i].audiotags [tagtype].tag != NULL) {
      slistSetStr (taglist, tagdefs [i].audiotags [tagtype].tag, tagdefs [i].tag);
    }
  }
}

static void
audiotagParseNumberPair (char *data, int *a, int *b)
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

static void
audiotagWriteMP3Tags (const char *ffn, slist_t *tagdata, int writetags)
{
  char        fn [MAXPATHLEN];
  char        track [50];
  char        disc [50];
  char        tmp [50];
  int         tagkey;
  slistidx_t  iteridx;
  char        *tag;
  char        *value;
  FILE        *ofh;

  logProcBegin (LOG_PROC, "audiotagsWriteMP3Tags");
  audiotagPreparePair (tagdata, track, sizeof (track),
      TAG_TRACKNUMBER, TAG_TRACKTOTAL, TAG_TYPE_MP3);
  audiotagPreparePair (tagdata, disc, sizeof (disc),
      TAG_DISCNUMBER, TAG_DISCTOTAL, TAG_TYPE_MP3);

  audiotagMakeTempFilename (fn, sizeof (fn));
  ofh = fopen (fn, "w");
  fprintf (ofh, "from mutagen.id3 import ID3,TXXX,UFID");
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].audiotags [TAG_TYPE_MP3].tag != NULL &&
        tagdefs [i].audiotags [TAG_TYPE_MP3].desc == NULL) {
      fprintf (ofh, ",%s", tagdefs [i].audiotags [TAG_TYPE_MP3].tag);
    }
  }
  fprintf (ofh, "\n");
  fprintf (ofh, "audio = ID3(\"%s\")\n", ffn);

  slistStartIterator (tagdata, &iteridx);
  while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
    tagkey = tagdefLookup (tag);
    if (tagkey < 0) {
      /* unknown tag */
      continue;
    }
    if (! tagdefs [tagkey].isNormTag && ! tagdefs [tagkey].isBDJTag) {
      continue;
    }
    if (writetags == WRITE_TAGS_BDJ_ONLY && tagdefs [tagkey].isNormTag) {
      continue;
    }

    value = slistGetStr (tagdata, tag);
    if (tagkey == TAG_TRACKNUMBER) {
      value = track;
    }
    if (tagkey == TAG_DISCNUMBER) {
      value = disc;
    }

    if (audiotagBDJ3CompatCheck (tmp, sizeof (tmp), tagkey, value)) {
      value = tmp;
    }

    if (tagkey == TAG_RECORDING_ID) {
      fprintf (ofh, "audio.delall('%s:%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc);
      fprintf (ofh, "audio.add(%s(encoding=3, owner=u'%s', data=b'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc, value);
    } else if (tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc != NULL) {
      fprintf (ofh, "audio.add(%s(encoding=3, desc=u'%s', text=u'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc, value);
    } else {
      fprintf (ofh, "audio.add(%s(encoding=3, text=u'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value);
    }
  }

  fprintf (ofh, "audio.save()\n");
  fclose (ofh);
  audiotagRunUpdate (fn);

  logProcEnd (LOG_PROC, "audiotagsWriteTags", "");
}

static void
audiotagWriteOtherTags (const char *ffn, slist_t *tagdata,
    int tagtype, int filetype, int writetags)
{
  char        fn [MAXPATHLEN];
  char        track [50];
  char        disc [50];
  char        tmp [50];
  int         tagkey;
  slistidx_t  iteridx;
  char        *tag;
  char        *value;
  FILE        *ofh;

  logProcBegin (LOG_PROC, "audiotagsWriteOtherTags");
  audiotagPreparePair (tagdata, track, sizeof (track),
      TAG_TRACKNUMBER, TAG_TRACKTOTAL, tagtype);
  audiotagPreparePair (tagdata, disc, sizeof (disc),
      TAG_DISCNUMBER, TAG_DISCTOTAL, tagtype);

  audiotagMakeTempFilename (fn, sizeof (fn));
  ofh = fopen (fn, "w");
  if (filetype == AFILE_TYPE_FLAC) {
    fprintf (ofh, "from mutagen.flac import FLAC\n");
    fprintf (ofh, "audio = FLAC(\"%s\")\n", ffn);
  }
  if (filetype == AFILE_TYPE_MPEG4) {
    fprintf (ofh, "from mutagen.mp4 import MP4\n");
    fprintf (ofh, "audio = MP4(\"%s\")\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGOPUS) {
    fprintf (ofh, "from mutagen.oggopus import OggOpus\n");
    fprintf (ofh, "audio = OggOpus(\"%s\")\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGVORBIS) {
    fprintf (ofh, "from mutagen.oggvorbis import OggVorbis\n");
    fprintf (ofh, "audio = OggVorbis(\"%s\")\n", ffn);
  }

  slistStartIterator (tagdata, &iteridx);
  while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
    tagkey = tagdefLookup (tag);
    if (tagkey < 0) {
      /* unknown tag  */
      continue;
    }
    if (! tagdefs [tagkey].isNormTag && ! tagdefs [tagkey].isBDJTag) {
      continue;
    }
    if (writetags == WRITE_TAGS_BDJ_ONLY && tagdefs [tagkey].isNormTag) {
      continue;
    }
    if (tagdefs [tagkey].audiotags [tagtype].tag == NULL) {
      /* not a supported tag for this audio tag type */
      continue;
    }

    value = slistGetStr (tagdata, tag);
    if (tagkey == TAG_TRACKNUMBER) {
      value = track;
    }
    if (tagkey == TAG_DISCNUMBER) {
      value = disc;
    }

    if (audiotagBDJ3CompatCheck (tmp, sizeof (tmp), tagkey, value)) {
      value = tmp;
    }

    if (tagtype == TAG_TYPE_MPEG4 &&
        tagdefs [tagkey].audiotags [tagtype].base != NULL) {
      fprintf (ofh, "audio[\"%s\"] = bytes ('%s', 'UTF-8')\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    } else {
      fprintf (ofh, "audio[\"%s\"] = u'%s'\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    }
  }

  fprintf (ofh, "audio.save()\n");
  fclose (ofh);
  audiotagRunUpdate (fn);

  logProcEnd (LOG_PROC, "audiotagsWriteOtherTags", "");
}

static void
audiotagPreparePair (slist_t *tagdata, char *buff, size_t sz,
    tagdefkey_t taga, tagdefkey_t tagb, int tagtype)
{
  char        *tdata;
  char        *tdatab;

  *buff = '\0';
  tdata = slistGetStr (tagdata, tagdefs [taga].tag);
  if (tdata != NULL) {
    strlcpy (buff, tdata, sz);
    tdatab = slistGetStr (tagdata, tagdefs [tagb].tag);
    if (tdatab != NULL) {
      if (tagtype == TAG_TYPE_MPEG4) {
        snprintf (buff, sz, "(%s,%s)", tdata, tdatab);
      } else {
        snprintf (buff, sz, "%s/%s", tdata, tdatab);
      }
    }
  }
}


static bool
audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value)
{
  bool    rc = false;

  if (bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS)) {
    if (tagkey == TAG_SONGSTART ||
        tagkey == TAG_SONGEND) {
      ssize_t   val;

      /* bdj3 song start/song end are stored as mm:ss.d */
      val = atoll (value);
      tmutilToMSD (val, tmp, sz);
      rc = true;
    }
    if (tagkey == TAG_VOLUMEADJUSTPERC) {
      double    val;

      val = atof (value);
      val *= 10;
      /* bdj3 volume adjust percentage is stored without a decimal point */
      snprintf (tmp, sz, "%ld", (long) val);
      rc = true;
    }
  }

  return rc;
}


static void
audiotagRunUpdate (const char *fn)
{
  const char  *targv [5];
  int         targc = 0;

  targv [targc++] = sysvarsGetStr (SV_PATH_PYTHON);
  targv [targc++] = fn;
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT | OS_PROC_DETACH, NULL, NULL);
  fileopDelete (fn);
}

static void
audiotagMakeTempFilename (char *fn, size_t sz)
{
  char        tbuff [MAXPATHLEN];

  snprintf (tbuff, sizeof (tbuff), "audiotag-%zd.py", globalCounter++);
  pathbldMakePath (fn, sz, tbuff, "", PATHBLD_MP_TMPDIR);
}
