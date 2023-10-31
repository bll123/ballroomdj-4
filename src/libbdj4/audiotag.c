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
#include "bdj4intl.h"
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

typedef struct {
  const char  *ext;
  int         tagtype;
  int         filetype;
} filetypelookup_t;

/* must be ascii sorted by extension */
filetypelookup_t filetypelookup [] = {
  { ".3g2",   TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".aac",   TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".alac",  TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".flac",  TAG_TYPE_VORBIS,  AFILE_TYPE_FLAC, },
  { ".m4a",   TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".m4r",   TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".mp3",   TAG_TYPE_ID3,     AFILE_TYPE_MP3, },
  { ".mp4",   TAG_TYPE_MP4,     AFILE_TYPE_MP4, },
  { ".oga",   TAG_TYPE_VORBIS,  AFILE_TYPE_VORBIS, },
  { ".ogg",   TAG_TYPE_VORBIS,  AFILE_TYPE_VORBIS, },
  /* .ogx files must be parsed to see what is in the container */
  { ".ogx",   TAG_TYPE_VORBIS,  AFILE_TYPE_OGG, },
  { ".opus",  TAG_TYPE_VORBIS,  AFILE_TYPE_OPUS, },
  { ".wav",   TAG_TYPE_WAV,     AFILE_TYPE_RIFF, },
  { ".wma",   TAG_TYPE_WMA,     AFILE_TYPE_ASF, },
};
enum {
  filetypelookupsz = sizeof (filetypelookup) / sizeof (filetypelookup_t),
};

static audiotag_t *at = NULL;

static void audiotagParseTags (slist_t *tagdata, const char *ffn, char *data, int filetype, int tagtype, int *rewrite);
static void audiotagCreateLookupTable (int tagtype);
static bool audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value);
static int  audiotagTagCheck (int writetags, int tagtype, const char *tag, int rewrite);
static void audiotagPrepareTotals (slist_t *tagdata, slist_t *newtaglist,
    nlist_t *datalist, int totkey, int tagkey);
static const char * audiotagTagLookup (int tagtype, const char *val);
static const char * audiotagTagName (int tagkey);
static const tagaudiotag_t *audiotagRawLookup (int tagkey, int tagtype);
static int  audiotagCompareExt (const void *ta, const void *tb);

void
audiotagInit (void)
{
  /* this is only an issue on linux when running an older install */
  /* on a newer system */
  if (! atiCheck (bdjoptGetStr (OPT_M_AUDIOTAG_INTFC))) {
    bdjoptSetStr (OPT_M_AUDIOTAG_INTFC, "libatibdj4");
  }

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

bool
audiotagUseReader (void)
{
  return atiUseReader (at->ati);
}

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
  audiotagParseTags (tagdata, ffn, data, filetype, tagtype, rewrite);
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
  const char  *tag;
  const char  *newvalue;
  const char  *value;
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
      tagtype != TAG_TYPE_ID3 &&
      tagtype != TAG_TYPE_MP4) {
    logProcEnd (LOG_PROC, "audiotagsWriteTags", "not-supported-tag");
    return AUDIOTAG_NOT_SUPPORTED;
  }

  if (filetype != AFILE_TYPE_OPUS &&
      filetype != AFILE_TYPE_VORBIS &&
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

    /* convert to bdj3 form before the update check */
    /* this works if the prior setting of bdj3-compat was off */
    /* but does not if the prior setting of bdj3-compat was on */
    if (audiotagBDJ3CompatCheck (tmp, sizeof (tmp), tagkey, newvalue)) {
      if (*tmp) {
        newvalue = tmp;
      } else {
        newvalue = NULL;
      }
    }

    if (tagdefs [tagkey].isBDJTag &&
        (rewrite & AF_FORCE_WRITE_BDJ) == AF_FORCE_WRITE_BDJ) {
      upd = true;
    }

    value = slistGetStr (tagdata, tag);
    if (! upd && newvalue != NULL && *newvalue && value == NULL) {
      upd = true;
    }
    if (! upd && newvalue != NULL && value != NULL &&
        *newvalue && strcmp (newvalue, value) != 0) {
      upd = true;
    }
    if (! upd && nlistGetNum (datalist, tagkey) == 1) {
      /* for track/disc total changes */
      upd = true;
    }

    /* special case */
    if (! upd && tagkey == TAG_RECORDING_ID &&
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

void *
audiotagSaveTags (const char *ffn)
{
  int   tagtype;
  int   filetype;
  void *sdata;

  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  sdata = atiSaveTags (at->ati, ffn, tagtype, filetype);
  return sdata;
}

void
audiotagFreeSavedTags (const char *ffn, void *sdata)
{
  int   tagtype;
  int   filetype;

  if (sdata == NULL) {
    return;
  }

  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  return atiFreeSavedTags (at->ati, sdata, tagtype, filetype);
}

int
audiotagRestoreTags (const char *ffn, void *sdata)
{
  int   tagtype;
  int   filetype;

  if (sdata == NULL) {
    return -1;
  }

  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  return atiRestoreTags (at->ati, sdata, ffn, tagtype, filetype);
}

void
audiotagCleanTags (const char *ffn)
{
  int   tagtype;
  int   filetype;

  audiotagDetermineTagType (ffn, &tagtype, &filetype);
  atiCleanTags (at->ati, ffn, tagtype, filetype);
}

void
audiotagDetermineTagType (const char *ffn, int *tagtype, int *filetype)
{
  pathinfo_t        *pi;
  char              tmp [MAXPATHLEN];
  filetypelookup_t  *ftl;
  filetypelookup_t  tftl = { NULL, 0, 0 };

  pi = pathInfo (ffn);

  *filetype = AFILE_TYPE_UNKNOWN;
  *tagtype = TAG_TYPE_VORBIS;

  snprintf (tmp, sizeof (tmp), "%.*s", (int) pi->elen, pi->extension);
  tftl.ext = tmp;
  ftl = bsearch (&tftl, filetypelookup, filetypelookupsz,
      sizeof (filetypelookup_t), audiotagCompareExt);

  if (ftl == NULL) {
    /* possible an alternate extension such as .original or .tmp */
    snprintf (tmp, sizeof (tmp), "%.*s", (int) pi->blen, pi->basename);
    pathInfoFree (pi);
    pi = pathInfo (tmp);
    snprintf (tmp, sizeof (tmp), "%.*s", (int) pi->elen, pi->extension);
    ftl = bsearch (&tftl, filetypelookup, filetypelookupsz,
        sizeof (filetypelookup_t), audiotagCompareExt);
  }

  if (ftl != NULL) {
    *tagtype = ftl->tagtype;
    *filetype = ftl->filetype;
  }

  pathInfoFree (pi);

  /* for .ogx files, check what codec is actually stored */
  /* if a file has a .ogg extension, the assumption is that it is */
  /*    ogg/vorbis, and .opus extensions are assumed to be ogg/opus */
  if (*filetype == AFILE_TYPE_OGG) {
    *filetype = atiCheckCodec (ffn, *filetype);
  }
}

/* internal routines */

static void
audiotagParseTags (slist_t *tagdata, const char *ffn, char *data,
    int filetype, int tagtype, int *rewrite)
{
  slistidx_t    iteridx;
  const char    *tag;

  atiParseTags (at->ati, tagdata, ffn, data, filetype, tagtype, rewrite);

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
      continue;
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
      if (tagdefs [i].audiotags [tagtype].desc != NULL) {
        /* for mp3: also add w/o the txxx= prefix */
        slistSetStr (taglist, tagdefs [i].audiotags [tagtype].desc, tagdefs [i].tag);
      } /* has a desc */
    } /* if there is a tag conversion */
  }
}

static bool
audiotagBDJ3CompatCheck (char *tmp, size_t sz, int tagkey, const char *value)
{
  bool    rc = false;

  if (value == NULL || ! *value) {
    return rc;
  }

  if (! bdjoptGetNum (OPT_G_BDJ3_COMPAT_TAGS)) {
    return rc;
  }

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
  const char  *newvalue;
  const char  *value;


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

  if (tagkey < 0 || tagkey >= TAG_KEY_MAX) {
    return NULL;
  }
  tagname = tagdefs [tagkey].tag;
  return tagname;
}

static const tagaudiotag_t *
audiotagRawLookup (int tagkey, int tagtype)
{
  if (tagkey < 0 || tagkey >= TAG_KEY_MAX) {
    return NULL;
  }
  return &tagdefs [tagkey].audiotags [tagtype];
}


static int
audiotagCompareExt (const void *ta, const void *tb)
{
  const filetypelookup_t    *a = ta;
  const filetypelookup_t    *b = tb;

  return strcmp (a->ext, b->ext);
}
