/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "ati.h"
#include "audiofile.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "osprocess.h"
#include "pathutil.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  AFILE_TYPE_UNKNOWN,
  AFILE_TYPE_FLAC,
  AFILE_TYPE_MP4,
  AFILE_TYPE_MP3,
  AFILE_TYPE_OGGOPUS,
  AFILE_TYPE_OGGVORBIS,
  AFILE_TYPE_WMA,
};

typedef struct audiotag {
  ati_t     *ati;
  slist_t   *tagTypeLookup [TAG_TYPE_MAX];
} audiotag_t;

static audiotag_t *at = NULL;

static void audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype);
static void audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite);
static void audiotagCreateLookupTable (int tagtype);
static int  audiotagWriteMP3Tags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int writetags);
static int  audiotagWriteOtherTags (const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype, int writetags);
static bool audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value);
static int  audiotagRunUpdate (const char *fn);
static void audiotagMakeTempFilename (char *fn, size_t sz);
static int  audiotagTagCheck (int writetags, int tagtype, const char *tag, int rewrite);
static void audiotagPrepareTotals (slist_t *tagdata, slist_t *newtaglist,
    nlist_t *datalist, int totkey, int tagkey);
static const char * audiotagTagLookup (int tagtype, const char *val);

static ssize_t globalCounter = 0;

void
audiotagInit (void)
{
  at = mdmalloc (sizeof (audiotag_t));
  at->ati = atiInit (bdjoptGetStr (OPT_M_AUDIOTAG_INTFC),
      audiotagTagLookup, audiotagTagCheck);

  for (int i = 0; i < TAG_TYPE_MAX; ++i) {
    at->tagTypeLookup [i] = NULL;
  }
}

void
audiotagCleanup (void)
{
  if (at == NULL) {
    return;
  }

  for (int i = 0; i < TAG_TYPE_MAX; ++i) {
    if (at->tagTypeLookup [i] != NULL) {
      slistFree (at->tagTypeLookup [i]);
      at->tagTypeLookup [i] = NULL;
    }
  }
  atiFree (at->ati);
  mdfree (at);
  at = NULL;
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
  return atiReadTags (at->ati, ffn);
}

slist_t *
audiotagParseData (const char *ffn, char *data, int *rewrite)
{
  slist_t     *tagdata;
  int         tagtype;
  int         filetype;

  *rewrite = 0;
  tagdata = slistAlloc ("atag", LIST_ORDERED, NULL);
  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  audiotagCreateLookupTable (tagtype);
  audiotagParseTags (tagdata, data, tagtype, rewrite);
  return tagdata;
}

int
audiotagWriteTags (const char *ffn, slist_t *tagdata, slist_t *newtaglist,
    int rewrite, int modTimeFlag)
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
  int         rc = AUDIOTAG_WRITE_OK; // if no update


  logProcBegin (LOG_PROC, "audiotagsWriteTags");

  writetags = bdjoptGetNum (OPT_G_WRITETAGS);
  if (writetags == WRITE_TAGS_NONE) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-tags-none");
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "write-none");
    return AUDIOTAG_WRITE_OFF;
  }

  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tagtype: %d afile-type: %d", tagtype, filetype);

  if (tagtype != TAG_TYPE_VORBIS &&
      tagtype != TAG_TYPE_MP3 &&
      tagtype != TAG_TYPE_MP4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-tag");
    return AUDIOTAG_NOT_SUPPORTED;
  }

  if (filetype != AFILE_TYPE_OGGOPUS &&
      filetype != AFILE_TYPE_OGGVORBIS &&
      filetype != AFILE_TYPE_FLAC &&
      filetype != AFILE_TYPE_MP3 &&
      filetype != AFILE_TYPE_MP4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-file");
    return AUDIOTAG_NOT_SUPPORTED;
  }

  datalist = nlistAlloc ("audiotag-data", LIST_ORDERED, NULL);
  nlistSetStr (datalist, TAG_TRACKTOTAL, "0");
  nlistSetStr (datalist, TAG_DISCTOTAL, "0");

  /* mp3 and mp4 store the track/disc totals in with the track */
  audiotagPrepareTotals (tagdata, newtaglist, datalist,
      TAG_TRACKTOTAL, TAG_TRACKNUMBER);
  audiotagPrepareTotals (tagdata, newtaglist, datalist,
      TAG_DISCTOTAL, TAG_DISCNUMBER);

  updatelist = slistAlloc ("audiotag-upd", LIST_ORDERED, NULL);
  dellist = slistAlloc ("audiotag-upd", LIST_ORDERED, NULL);
  slistStartIterator (newtaglist, &iteridx);
  while ((tag = slistIterateKey (newtaglist, &iteridx)) != NULL) {
    bool  upd;

    upd = false;
    tagkey = audiotagTagCheck (writetags, tagtype, tag, rewrite);
    if (tagkey < 0) {
      continue;
    }

    newvalue = slistGetStr (newtaglist, tag);

    if (tagdefs [tagkey].isBDJTag &&
        (rewrite & AF_FORCE_WRITE_BDJ) == AF_FORCE_WRITE_BDJ) {
      upd = true;
    }

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
      upd = true;
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
    tagkey = audiotagTagCheck (writetags, tagtype, tag, rewrite);
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

  /* special cases */
  if ((rewrite & AF_REWRITE_VARIOUS) == AF_REWRITE_VARIOUS) {
    slistSetNum (dellist, "VARIOUSARTISTS", 0);
  }
  if ((rewrite & AF_REWRITE_DURATION) == AF_REWRITE_DURATION) {
    slistSetNum (dellist, "DURATION", 0);
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
    time_t    origtm;

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "writing tags");
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  %s", ffn);
    origtm = fileopModTime (ffn);
    if (tagtype == TAG_TYPE_MP3 && filetype == AFILE_TYPE_MP3) {
      rc = audiotagWriteMP3Tags (ffn, updatelist, dellist, datalist, writetags);
    } else {
      rc = audiotagWriteOtherTags (ffn, updatelist, dellist, datalist, tagtype, filetype, writetags);
    }
    if (modTimeFlag == AT_KEEP_MOD_TIME) {
      fileopSetModTime (ffn, origtm);
    }
  }
  slistFree (updatelist);
  slistFree (dellist);
  nlistFree (datalist);
  logProcEnd (LOG_PROC, "audiotagsWriteTags", "");
  return rc;
}

static void
audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype)
{
  pathinfo_t  *pi;
  char        tmp [MAXPATHLEN];
  bool        found = false;

  pi = pathInfo (ffn);

  *filetype = AFILE_TYPE_UNKNOWN;
  *tagtype = TAG_TYPE_VORBIS;

  if (pathInfoExtCheck (pi, ".mp3") ||
      pathInfoExtCheck (pi, ".m4a") ||
      pathInfoExtCheck (pi, ".wma") ||
      pathInfoExtCheck (pi, ".ogg") ||
      pathInfoExtCheck (pi, ".opus") ||
      pathInfoExtCheck (pi, ".flac")) {
    found = true;
  }

  if (! found) {
    /* this handles .mp3.original situations */
    snprintf (tmp, sizeof (tmp), "%.*s", (int) pi->blen, pi->basename);
    pathInfoFree (pi);
    pi = pathInfo (tmp);
  }

  if (pathInfoExtCheck (pi, ".mp3")) {
    *tagtype = TAG_TYPE_MP3;
    *filetype = AFILE_TYPE_MP3;
  }
  if (pathInfoExtCheck (pi, ".m4a")) {
    *tagtype = TAG_TYPE_MP4;
    *filetype = AFILE_TYPE_MP4;
  }
  if (pathInfoExtCheck (pi, ".wma")) {
    *tagtype = TAG_TYPE_WMA;
    *filetype = AFILE_TYPE_WMA;
  }
  if (pathInfoExtCheck (pi, ".ogg")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGVORBIS;
  }
  if (pathInfoExtCheck (pi, ".opus")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGOPUS;
  }
  if (pathInfoExtCheck (pi, ".flac")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_FLAC;
  }

  pathInfoFree (pi);
}

static void
audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite)
{
  atiParseTags (at->ati, tagdata, data, tagtype, rewrite);
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

  if (at->tagTypeLookup [tagtype] != NULL) {
    return;
  }

  snprintf (buff, sizeof (buff), "tag-%d", tagtype);
  at->tagTypeLookup [tagtype] = slistAlloc (buff, LIST_ORDERED, NULL);
  taglist = at->tagTypeLookup [tagtype];

  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (! tagdefs [i].isNormTag && ! tagdefs [i].isBDJTag) {
      continue;
    }
    if (tagdefs [i].audiotags [tagtype].tag != NULL) {
      slistSetStr (taglist, tagdefs [i].audiotags [tagtype].tag, tagdefs [i].tag);
    }
  }
}

static int
audiotagWriteMP3Tags (const char *ffn, slist_t *updatelist, slist_t *dellist,
    nlist_t *datalist, int writetags)
{
  char        fn [MAXPATHLEN];
  int         tagkey;
  slistidx_t  iteridx;
  char        *tag;
  char        *value;
  FILE        *ofh;
  int         rc;

  logProcBegin (LOG_PROC, "audiotagsWriteMP3Tags");

  audiotagMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  fprintf (ofh, "from mutagen.id3 import ID3,TXXX,UFID");
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    if (tagdefs [i].audiotags [TAG_TYPE_MP3].tag != NULL &&
        tagdefs [i].audiotags [TAG_TYPE_MP3].desc == NULL) {
      fprintf (ofh, ",%s", tagdefs [i].audiotags [TAG_TYPE_MP3].tag);
    }
  }
  fprintf (ofh, ",ID3NoHeaderError\n");
  fprintf (ofh, "try:\n");
  fprintf (ofh, "  audio = ID3('%s')\n", ffn);

  slistStartIterator (dellist, &iteridx);
  while ((tag = slistIterateKey (dellist, &iteridx)) != NULL) {
    /* special cases - old audio tags */
    if (strcmp (tag, "VARIOUSARTISTS") == 0) {
      fprintf (ofh, "  audio.delall('TXXX:VARIOUSARTISTS')\n");
      continue;
    }
    if (strcmp (tag, "DURATION") == 0) {
      fprintf (ofh, "  audio.delall('TXXX:DURATION')\n");
      continue;
    }

    tagkey = audiotagTagCheck (writetags, TAG_TYPE_MP3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    if (tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc != NULL) {
      fprintf (ofh, "  audio.delall('%s:%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc);
    } else {
      fprintf (ofh, "  audio.delall('%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, TAG_TYPE_MP3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write: %s %s",
        tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value);
    if (tagkey == TAG_RECORDING_ID) {
      fprintf (ofh, "  audio.delall('%s:%s')\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc);
      fprintf (ofh, "  audio.add(%s(encoding=3, owner=u'%s', data=b'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc, value);
    } else if (tagkey == TAG_TRACKNUMBER ||
        tagkey == TAG_DISCNUMBER) {
      if (value != NULL && *value) {
        const char  *tot = NULL;

        if (tagkey == TAG_TRACKNUMBER) {
          tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
        }
        if (tagkey == TAG_DISCNUMBER) {
          tot = nlistGetStr (datalist, TAG_DISCTOTAL);
        }
        if (tot != NULL) {
          fprintf (ofh, "  audio.add(%s(encoding=3, text=u'%s/%s'))\n",
              tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value, tot);
        } else {
          fprintf (ofh, "  audio.add(%s(encoding=3, text=u'%s'))\n",
              tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value);
        }
      }
    } else if (tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc != NULL) {
      fprintf (ofh, "  audio.add(%s(encoding=3, desc=u'%s', text=u'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].base,
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].desc, value);
    } else {
      fprintf (ofh, "  audio.add(%s(encoding=3, text=u'%s'))\n",
          tagdefs [tagkey].audiotags [TAG_TYPE_MP3].tag, value);
    }
  }

  fprintf (ofh, "  audio.save()\n");
  fprintf (ofh, "except ID3NoHeaderError as e:\n");
  fprintf (ofh, "  exit (1)\n");
  fclose (ofh);

  rc = audiotagRunUpdate (fn);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  logProcEnd (LOG_PROC, "audiotagsWriteMP3Tags", "");
  return rc;
}

static int
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
  int         rc;

  logProcBegin (LOG_PROC, "audiotagsWriteOtherTags");

  audiotagMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (filetype == AFILE_TYPE_FLAC) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: flac");
    fprintf (ofh, "from mutagen.flac import FLAC\n");
    fprintf (ofh, "audio = FLAC('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: mp4");
    fprintf (ofh, "from mutagen.mp4 import MP4\n");
    fprintf (ofh, "audio = MP4('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGOPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: opus");
    fprintf (ofh, "from mutagen.oggopus import OggOpus\n");
    fprintf (ofh, "audio = OggOpus('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGGVORBIS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: oggvorbis");
    fprintf (ofh, "from mutagen.oggvorbis import OggVorbis\n");
    fprintf (ofh, "audio = OggVorbis('%s')\n", ffn);
  }

  slistStartIterator (dellist, &iteridx);
  while ((tag = slistIterateKey (dellist, &iteridx)) != NULL) {
    /* special cases - old audio tags */
    if (strcmp (tag, "VARIOUSARTISTS") == 0) {
      fprintf (ofh, "audio.pop('VARIOUSARTISTS')\n");
      continue;
    }
    if (strcmp (tag, "DURATION") == 0) {
      fprintf (ofh, "audio.pop('DURATION')\n");
      continue;
    }

    tagkey = audiotagTagCheck (writetags, tagtype, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }
    fprintf (ofh, "audio.pop('%s')\n",
        tagdefs [tagkey].audiotags [tagtype].tag);
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = audiotagTagCheck (writetags, tagtype, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write: %s %s",
        tagdefs [tagkey].audiotags [tagtype].tag, value);
    if (tagtype == TAG_TYPE_MP4 &&
        tagkey == TAG_BPM) {
      fprintf (ofh, "audio['%s'] = [%s]\n",
          tagdefs [tagkey].audiotags [tagtype].tag, value);
    } else if (tagtype == TAG_TYPE_MP4 &&
        (tagkey == TAG_TRACKNUMBER ||
        tagkey == TAG_DISCNUMBER)) {
      if (value != NULL && *value) {
        const char  *tot = NULL;

        if (tagkey == TAG_TRACKNUMBER) {
          tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
        }
        if (tagkey == TAG_DISCNUMBER) {
          tot = nlistGetStr (datalist, TAG_DISCTOTAL);
        }
        if (tot == NULL || *tot == '\0') {
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

  rc = audiotagRunUpdate (fn);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  logProcEnd (LOG_PROC, "audiotagsWriteOtherTags", "");
  return rc;
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
      tmutilToMSD (val, tmp, sz, 1);
      rc = true;
    }
    if (tagkey == TAG_VOLUMEADJUSTPERC) {
      double    val;
      char      *radix;
      char      *tptr;

      val = atof (value);
      val /= DF_DOUBLE_MULT;
      val *= 10.0;
      /* bdj3 volume adjust percentage should be stored */
      /* with a decimal point if possible */
      snprintf (tmp, sz, "%.2f", val);
      radix = sysvarsGetStr (SV_LOCALE_RADIX);
      tptr = strstr (tmp, radix);
      if (tptr != NULL) {
        *tptr = '.';
      }
      rc = true;
    }
  }

  return rc;
}

static int
audiotagRunUpdate (const char *fn)
{
  const char  *targv [5];
  int         targc = 0;
  int         rc;
  char        dbuff [4096];

  targv [targc++] = sysvarsGetStr (SV_PATH_PYTHON);
  targv [targc++] = fn;
  targv [targc++] = NULL;
  /* the wait flag is on, the return code is the process return code */
  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, dbuff, sizeof (dbuff), NULL);
  if (rc == 0) {
    fileopDelete (fn);
  } else {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write tags failed %d (%s)", rc, fn);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  output: %s", dbuff);
  }
  return rc;
}

static void
audiotagMakeTempFilename (char *fn, size_t sz)
{
  char        tbuff [MAXPATHLEN];

  snprintf (tbuff, sizeof (tbuff), "audiotag-%"PRId64".py", (int64_t) globalCounter++);
  pathbldMakePath (fn, sz, tbuff, "", PATHBLD_MP_DREL_TMP);
}

static int
audiotagTagCheck (int writetags, int tagtype, const char *tag, int rewrite)
{
  int tagkey = -1;

  tagkey = tagdefLookup (tag);
  if (tagkey < 0) {
    /* unknown tag  */
    // logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unknown-tag: %s", tag);
    return tagkey;
  }
  if (! tagdefs [tagkey].isNormTag && ! tagdefs [tagkey].isBDJTag) {
    // logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "not-written: %s", tag);
    return -1;
  }
  if (writetags == WRITE_TAGS_BDJ_ONLY && tagdefs [tagkey].isNormTag) {
    if ((rewrite & AF_FORCE_WRITE_BDJ) != AF_FORCE_WRITE_BDJ) {
      // logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "bdj-only: %s", tag);
      return -1;
    }
  }
  if (tagdefs [tagkey].audiotags [tagtype].tag == NULL) {
    /* not a supported tag for this audio tag type */
    // logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "unsupported: %s", tag);
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

static const char *
audiotagTagLookup (int tagtype, const char *val)
{
  const char  *tagname;

  tagname = slistGetStr (at->tagTypeLookup [tagtype], val);
  return tagname;
}
