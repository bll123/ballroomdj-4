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
#include "nlist.h"
#include "osprocess.h"
#include "pathutil.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  AF_REWRITE_NONE = 0x0000,
  AF_REWRITE_MB   = 0x0001,
};

enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MP4,
  AFILE_TYPE_MP3,
  AFILE_TYPE_OGGOPUS,
  AFILE_TYPE_OGGVORBIS,
  AFILE_TYPE_WMA,
};

static void audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype);
static void audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite);
static void audiotagCreateLookupTable (int tagtype);
static void audiotagParseNumberPair (char *data, int *a, int *b);
static void audiotagWriteMP3Tags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int writetags);
static void audiotagWriteOtherTags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int writetags);
static bool audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value);
static void audiotagRunUpdate (const char *fn);
static void audiotagMakeTempFilename (char *fn, size_t sz);
static int  audiotagTagCheck (int writetags, int tagtype, const char *tag);
static void audiotagPrepareTotals (slist_t *tagdata, slist_t *newtaglist,
    nlist_t *datalist, int totkey, int tagkey);

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
audiotagParseData (const char *ffn, char *data, int *rewrite)
{
  slist_t     *tagdata;
  int         tagtype;
  int         filetype;

  *rewrite = 0;
  tagdata = slistAlloc ("atag", LIST_ORDERED, free);
  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  audiotagParseTags (tagdata, data, tagtype, rewrite);
  return tagdata;
}

void
audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist,
    int rewrite)
{
  char        tmp [50];
  int         tagtype;
  int         filetype;
  int         writetags;
  slistidx_t  iteridx;
  char        *tag;
  char        *newvalue;
  char        *value;
  slist_t     *updatelist;
  slist_t     *dellist;
  nlist_t     *datalist;
  int         tagkey;


  logProcBegin (LOG_PROC, "audiotagsWriteTags");

  writetags = bdjoptGetNum (OPT_G_WRITETAGS);
  if (writetags == WRITE_TAGS_NONE) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "write-tags-none");
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "write-none");
    return;
  }

  audiotagDetermineTagType (ffn, &tagtype, &filetype);

  if (tagtype != TAG_TYPE_VORBIS &&
      tagtype != TAG_TYPE_MP3 &&
      tagtype != TAG_TYPE_MP4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-tag");
    return;
  }

  if (filetype != AFILE_TYPE_OGGOPUS &&
      filetype != AFILE_TYPE_OGGVORBIS &&
      filetype != AFILE_TYPE_FLAC &&
      filetype != AFILE_TYPE_MP3 &&
      filetype != AFILE_TYPE_MP4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-file");
    return;
  }

  datalist = nlistAlloc ("audiotag-data", LIST_ORDERED, free);
  nlistSetStr (datalist, TAG_TRACKTOTAL, "0");
  nlistSetStr (datalist, TAG_DISCTOTAL, "0");

  /* mp3 and mp4 store the track/disc totals in with the track */
  audiotagPrepareTotals (tagdata, newtaglist, datalist,
      TAG_TRACKTOTAL, TAG_TRACKNUMBER);
  audiotagPrepareTotals (tagdata, newtaglist, datalist,
      TAG_DISCTOTAL, TAG_DISCNUMBER);

  updatelist = slistAlloc ("audiotag-upd", LIST_ORDERED, free);
  dellist = slistAlloc ("audiotag-upd", LIST_ORDERED, NULL);
  slistStartIterator (newtaglist, &iteridx);
  while ((tag = slistIterateKey (newtaglist, &iteridx)) != NULL) {
    bool  upd;

    upd = false;
    tagkey = audiotagTagCheck (writetags, tagtype, tag);
    if (tagkey < 0) {
      continue;
    }

    newvalue = slistGetStr (newtaglist, tag);

    value = slistGetStr (tagdata, tag);
    if (newvalue != NULL && *newvalue && value == NULL) {
      upd = true;
    }
    if (newvalue != NULL && value != NULL &&
        *newvalue && strcmp (newvalue, value) != 0) {
      upd = true;
    }
    if (nlistGetNum (datalist, tagkey) == 1) {
      /* for track/disc total changes */
      upd = true;
    }

    /* convert to bdj3 form after the update check */
    if (audiotagBDJ3CompatCheck (tmp, sizeof (tmp), tagkey, newvalue)) {
      newvalue = tmp;
    }

    /* special case */
    if (tagkey == TAG_RECORDING_ID &&
        (rewrite & AF_REWRITE_MB) == AF_REWRITE_MB &&
        newvalue != NULL && *newvalue) {
      upd = true;
    }

    if (upd) {
      slistSetStr (updatelist, tag, newvalue);
    }
  }

  slistStartIterator (tagdata, &iteridx);
  while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, tagtype, tag);
    if (tagkey < 0) {
      continue;
    }

    newvalue = slistGetStr (newtaglist, tag);
    if (newvalue == NULL) {
      slistSetNum (dellist, tag, 0);
    } else {
      value = slistGetStr (tagdata, tag);
      if (! *newvalue && *value) {
        slistSetNum (dellist, tag, 0);
      }
    }
  }

  /* special case */
  if ((rewrite & AF_REWRITE_MB) == AF_REWRITE_MB) {
    newvalue = slistGetStr (newtaglist, tagdefs [TAG_RECORDING_ID].tag);
    value = slistGetStr (tagdata, tagdefs [TAG_RECORDING_ID].tag);
    if ((newvalue == NULL || ! *newvalue) && ! *value) {
      slistSetNum (dellist, tag, 0);
    }
  }

  if (slistGetCount (updatelist) > 0 ||
      slistGetCount (dellist) > 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "writing tags");
    logMsg (LOG_DBG, LOG_DBUPDATE, "  %s", ffn);
    if (tagtype == TAG_TYPE_MP3 && filetype == AFILE_TYPE_MP3) {
      audiotagWriteMP3Tags (ffn, updatelist, dellist, datalist, writetags);
    } else {
      audiotagWriteOtherTags (ffn, updatelist, dellist, datalist, tagtype, filetype, writetags);
    }
  }
  slistFree (updatelist);
  slistFree (dellist);
  nlistFree (datalist);
  logProcEnd (LOG_PROC, "audiotagsWriteTags", "");
}

static void
audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype)
{
  pathinfo_t  *pi;
  char        tmp [40];

  pi = pathInfo (ffn);

  *tmp = '\0';
  if (pi->elen > 0 && pi->elen + 1 < sizeof (tmp)) {
    strlcpy (tmp, pi->extension, pi->elen + 1);
  }

  *filetype = AFILE_TYPE_UNKNOWN;
  *tagtype = TAG_TYPE_VORBIS;
  if (strcmp (tmp, ".mp3") == 0) {
    *tagtype = TAG_TYPE_MP3;
    *filetype = AFILE_TYPE_MP3;
  }
  if (strcmp (tmp, ".m4a") == 0) {
    *tagtype = TAG_TYPE_MP4;
    *filetype = AFILE_TYPE_MP4;
  }
  if (strcmp (tmp, ".wma") == 0) {
    *tagtype = TAG_TYPE_WMA;
    *filetype = AFILE_TYPE_WMA;
  }
  if (strcmp (tmp, ".ogg") == 0) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGVORBIS;
  }
  if (strcmp (tmp, ".opus") == 0) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGOPUS;
  }
  if (strcmp (tmp, ".flac") == 0) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_FLAC;
  }

  pathInfoFree (pi);
}

static void
audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite)
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
  bool      skip;
  int       tagkey;
  int       writetags;

  writetags = bdjoptGetNum (OPT_G_WRITETAGS);
  audiotagCreateLookupTable (tagtype);

/*
 * mutagen output:
 *
 * mp4 output from mutagen is very bizarre for freeform tags
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
          p = "UFID=http://musicbrainz.org";
        }
      }

      /* p is pointing to the tag name */
      tagname = slistGetStr (tagLookup [tagtype], p);
      if (tagname != NULL) {
        tagkey = audiotagTagCheck (writetags, tagtype, tagname);
      }
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

        if (strcmp (tagname, tagdefs [TAG_DURATION].tag) == 0) {
          if (haveduration) {
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
            tm /= 10.0;
            snprintf (pbuff, sizeof (pbuff), "%.2f", tm);
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
audiotagWriteMP3Tags (const char *ffn, slist_t *updatelist, slist_t *dellist,
    nlist_t *datalist, int writetags)
{
  char        fn [MAXPATHLEN];
  int         tagkey;
  slistidx_t  iteridx;
  char        *tag;
  char        *value;
  FILE        *ofh;

  logProcBegin (LOG_PROC, "audiotagsWriteMP3Tags");

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
  fprintf (ofh, "audio = ID3('%s')\n", ffn);

  slistStartIterator (dellist, &iteridx);
  while ((tag = slistIterateKey (dellist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, TAG_TYPE_MP3, tag);
    if (tagkey < 0) {
      continue;
    }

    if (tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc != NULL) {
      fprintf (ofh, "audio.delall('%s:%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc);
    } else {
      fprintf (ofh, "audio.delall('%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, TAG_TYPE_MP3, tag);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    logMsg (LOG_DBG, LOG_DBUPDATE, "  write: %s %s",
        tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value);
    if (tagkey == TAG_RECORDING_ID) {
      fprintf (ofh, "audio.delall('%s:%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc);
      fprintf (ofh, "audio.add(%s(encoding=3, owner=u'%s', data=b'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc, value);
    } else if (tagkey == TAG_TRACKNUMBER) {
      if (value != NULL && *value) {
        fprintf (ofh, "audio.add(%s(encoding=3, text=u'%s/%s'))\n",
            tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value,
            nlistGetStr (datalist, TAG_TRACKTOTAL));
      }
    } else if (tagkey == TAG_DISCNUMBER) {
      if (value != NULL && *value) {
        fprintf (ofh, "audio.add(%s(encoding=3, text=u'%s/%s'))\n",
            tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value,
            nlistGetStr (datalist, TAG_DISCTOTAL));
      }
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
audiotagWriteOtherTags (const char *ffn, slist_t *updatelist,
    slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype, int writetags)
{
  char        fn [MAXPATHLEN];
  int         tagkey;
  slistidx_t  iteridx;
  char        *tag;
  char        *value;
  FILE        *ofh;

  logProcBegin (LOG_PROC, "audiotagsWriteOtherTags");

  audiotagMakeTempFilename (fn, sizeof (fn));
  ofh = fopen (fn, "w");
  if (filetype == AFILE_TYPE_FLAC) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "file-type: flac");
    fprintf (ofh, "from mutagen.flac import FLAC\n");
    fprintf (ofh, "audio = FLAC('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "file-type: mp4");
    fprintf (ofh, "from mutagen.mp4 import MP4\n");
    fprintf (ofh, "audio = MP4('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGOPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "file-type: opus");
    fprintf (ofh, "from mutagen.oggopus import OggOpus\n");
    fprintf (ofh, "audio = OggOpus('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGVORBIS) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "file-type: oggvorbis");
    fprintf (ofh, "from mutagen.oggvorbis import OggVorbis\n");
    fprintf (ofh, "audio = OggVorbis('%s')\n", ffn);
  }

  slistStartIterator (dellist, &iteridx);
  while ((tag = slistIterateKey (dellist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, tagtype, tag);
    if (tagkey < 0) {
      continue;
    }
    fprintf (ofh, "audio.pop('%s')\n",
        tagdefs [tagkey].audiotags [tagtype].tag);
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, tagtype, tag);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    logMsg (LOG_DBG, LOG_DBUPDATE, "  write: %s %s",
        tagdefs [tagkey].audiotags [tagtype].tag, value);
    if (tagtype == TAG_TYPE_MP4 &&
        tagkey == TAG_BPM) {
      fprintf (ofh, "audio['%s'] = [%s]\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    } else if (tagtype == TAG_TYPE_MP4 &&
        tagkey == TAG_TRACKNUMBER) {
      if (value != NULL && *value) {
        const char  *tot;

        tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
        if (tot == NULL) {
          tot = "0";
        }
        fprintf (ofh, "audio['%s'] = [(%s,%s)]\n",
            tagdefs [tagkey].audiotags [tagtype].tag, value, tot);
      }
    } else if (tagtype == TAG_TYPE_MP4 &&
        tagkey == TAG_DISCNUMBER) {
      if (value != NULL && *value) {
        const char  *tot;

        tot = nlistGetStr (datalist, TAG_DISCTOTAL);
        if (tot == NULL) {
          tot = "0";
        }
        fprintf (ofh, "audio['%s'] = [(%s,%s)]\n",
            tagdefs [tagkey].audiotags [tagtype].tag, value, tot);
      }
    } else if (tagtype == TAG_TYPE_MP4 &&
        tagdefs [tagkey].audiotags [tagtype].base != NULL) {
      fprintf (ofh, "audio['%s'] = bytes ('%s', 'UTF-8')\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    } else {
      fprintf (ofh, "audio['%s'] = u'%s'\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    }
  }

  fprintf (ofh, "audio.save()\n");
  fclose (ofh);
  audiotagRunUpdate (fn);

  logProcEnd (LOG_PROC, "audiotagsWriteOtherTags", "");
}

static bool
audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value)
{
  bool    rc = false;

  if (value == NULL || ! *value) {
    return rc;
  }

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
      val *= 10.0;
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

static int
audiotagTagCheck (int writetags, int tagtype, const char *tag)
{
  int tagkey = -1;

  tagkey = tagdefLookup (tag);
  if (tagkey < 0) {
    /* unknown tag  */
    //logMsg (LOG_DBG, LOG_DBUPDATE, "unknown-tag: %s", tag);
    return tagkey;
  }
  if (! tagdefs [tagkey].isNormTag && ! tagdefs [tagkey].isBDJTag) {
    //logMsg (LOG_DBG, LOG_DBUPDATE, "not-written: %s", tag);
    return -1;
  }
  if (writetags == WRITE_TAGS_BDJ_ONLY && tagdefs [tagkey].isNormTag) {
    //logMsg (LOG_DBG, LOG_DBUPDATE, "bdj-only: %s\n", tag);
    return -1;
  }
  if (tagdefs [tagkey].audiotags [tagtype].tag == NULL) {
    /* not a supported tag for this audio tag type */
    //logMsg (LOG_DBG, LOG_DBUPDATE, "unsupported: %s", tag);
    return -1;
  }

  return tagkey;
}

static void
audiotagPrepareTotals (slist_t *tagdata, slist_t *newtaglist,
    nlist_t *datalist, int totkey, int tagkey)
{
  const char  *tag;
  char        *newvalue;
  char        *value;


  tag = tagdefs [totkey].tag;
  newvalue = slistGetStr (newtaglist, tag);
  value = slistGetStr (tagdata, tag);
  if (newvalue != NULL && *newvalue && value == NULL) {
    nlistSetNum (datalist, tagkey, 1);
  }
  if (newvalue != NULL && value != NULL &&
      strcmp (newvalue, value) != 0) {
    nlistSetNum (datalist, tagkey, 1);
  }
  nlistSetStr (datalist, totkey, newvalue);
}
