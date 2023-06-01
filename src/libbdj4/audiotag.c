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

#include "ati.h"
#include "audiofile.h"
#include "audiotag.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

typedef struct audiotag {
  ati_t     *ati;
  slist_t   *tagTypeLookup [TAG_TYPE_MAX];
} audiotag_t;

static audiotag_t *at = NULL;

static void audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype);
static void audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite);
static void audiotagCreateLookupTable (int tagtype);
static bool audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value);
static int  audiotagTagCheck (int writetags, int tagtype, const char *tag, int rewrite);
static void audiotagPrepareTotals (slist_t *tagdata, slist_t *newtaglist,
    nlist_t *datalist, int totkey, int tagkey);
static const char * audiotagTagLookup (int tagtype, const char *val);
static const char * audiotagTagName (int tagkey);
static const tagaudiotag_t *audiotagRawLookup (int tagkey, int tagtype);

void
audiotagInit (void)
{
  at = mdmalloc (sizeof (audiotag_t));
  at->ati = atiInit (bdjoptGetStr (OPT_M_AUDIOTAG_INTFC),
      bdjoptGetNum (OPT_G_WRITETAGS),
      audiotagTagLookup, audiotagTagCheck,
      audiotagTagName, audiotagRawLookup);

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


  logProcBegin (LOG_PROC, "audiotagWriteTags");

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
    value = slistGetStr (tagdata, tagdefs [TAG_RECORDING_ID].tag);
    newvalue = slistGetStr (newtaglist, tagdefs [TAG_RECORDING_ID].tag);
    if ((newvalue == NULL || ! *newvalue) && value != NULL && *value) {
      slistSetNum (dellist, tag, 0);
    }
  }

  if (slistGetCount (updatelist) > 0 ||
      slistGetCount (dellist) > 0) {
    time_t    origtm;

    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "writing tags");
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  %s", ffn);
    origtm = fileopModTime (ffn);

    atiWriteTags (at->ati, ffn, updatelist, dellist, datalist,
        tagtype, filetype);

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
  } else if (pathInfoExtCheck (pi, ".m4a")) {
    *tagtype = TAG_TYPE_MP4;
    *filetype = AFILE_TYPE_MP4;
  } else if (pathInfoExtCheck (pi, ".wma")) {
    *tagtype = TAG_TYPE_WMA;
    *filetype = AFILE_TYPE_WMA;
  } else if (pathInfoExtCheck (pi, ".ogg")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGVORBIS;
  } else if (pathInfoExtCheck (pi, ".opus")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_OGGOPUS;
  } else if (pathInfoExtCheck (pi, ".flac")) {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_FLAC;
  } else {
    *tagtype = TAG_TYPE_VORBIS;
    *filetype = AFILE_TYPE_UNKNOWN;
  }

  pathInfoFree (pi);
}

static void
audiotagParseTags (slist_t *tagdata, char *data, int tagtype, int *rewrite)
{
  slistidx_t    iteridx;
  const char    *tag;

  atiParseTags (at->ati, tagdata, data, tagtype, rewrite);

  slistStartIterator (tagdata, &iteridx);
  while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
    /* old songend/songstart handling */
    if (strcmp (tag, tagdefs [TAG_SONGSTART].tag) == 0 ||
        strcmp (tag, tagdefs [TAG_SONGEND].tag) == 0) {
      const char  *tagval;
      char        *p;
      char        *tokstr;
      char        *tmp;
      char        pbuff [40];
      double      tm = 0.0;

      tagval = slistGetStr (tagdata, tag);
      if (strstr (tagval, ":") != NULL) {
        tmp = mdstrdup (tagval);
        p = strtok_r (tmp, ":", &tokstr);
        if (p != NULL) {
          tm += atof (p) * 60.0;
          p = strtok_r (NULL, ":", &tokstr);
          tm += atof (p);
          tm *= 1000;
          snprintf (pbuff, sizeof (pbuff), "%.0f", tm);
          slistSetStr (tagdata, tag, pbuff);
        }
        mdfree (tmp);
      }
    }

    /* old volumeadjustperc handling */
    if (strcmp (tag, tagdefs [TAG_VOLUMEADJUSTPERC].tag) == 0) {
      const char  *tagval;
      const char  *radix;
      char        *tmp;
      char        *p;
      char        pbuff [40];
      double      tm = 0.0;

      tagval = slistGetStr (tagdata, tag);
      radix = sysvarsGetStr (SV_LOCALE_RADIX);

      /* the BDJ3 volume adjust percentage is a double */
      /* with or without a decimal point */
      /* convert it to BDJ4 style */
      /* this will fail for large BDJ3 values w/no decimal */
      if (strstr (tagval, ".") != NULL || strlen (tagval) <= 3) {
        tmp = mdstrdup (tagval);
        p = strstr (tmp, ".");
        if (p != NULL) {
          if (radix != NULL) {
            *p = *radix;
          }
        }
        tm = atof (tagval);
        tm /= 10.0;
        tm *= DF_DOUBLE_MULT;
        snprintf (pbuff, sizeof (pbuff), "%.0f", tm);
        slistSetStr (tagdata, tag, pbuff);
        mdfree (tmp);
      }
    }
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

/* returns the tag name given the tagtype specific raw name */
/* the atimutagen library needs this process */
static const char *
audiotagTagLookup (int tagtype, const char *val)
{
  const char  *tagname;

  audiotagCreateLookupTable (tagtype);
  tagname = slistGetStr (at->tagTypeLookup [tagtype], val);
  return tagname;
}

/* returns the tag name given the tag identifier constant */
static const char *
audiotagTagName (int tagkey)
{
  const char  *tagname;

  tagname = tagdefs [tagkey].tag;
  return tagname;
}

static const tagaudiotag_t *
audiotagRawLookup (int tagkey, int tagtype)
{
  return &tagdefs [tagkey].audiotags [tagtype];
}
