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
#include <errno.h>

#include "ati.h"
#include "atioggutil.h"
#include "audiofile.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "pathbld.h"
#include "sysvars.h"
#include "tagdef.h"

typedef struct atidata {
  const char        *python;
  const char        *mutagen;
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
  bdjregex_t        *ptre;
  bdjregex_t        *encre;
  bdjregex_t        *tagre;
} atidata_t;

static ssize_t globalCounter = 0;

static int  atimutagenWriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist);
static int  atimutagenWriteOtherTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static void atimutagenMakeTempFilename (char *fn, size_t sz);
static int  atimutagenRunUpdate (const char *fn, char *buff, size_t sz);
static void atimutagenWritePythonHeader (atidata_t *atidata, const char *ffn, FILE *ofh, int tagtype, int filetype);
static void atimutagenWritePythonTrailer (FILE *ofh, int tagtype, int filetype);

typedef struct atisaved {
  bool        hasdata;
  int         filetype;
  int         tagtype;
  char        *data;
  size_t      dlen;
} atisaved_t;

const char *
atiiDesc (void)
{
  return "Mutagen";
}

atidata_t *
atiiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  atidata_t *atidata;

  atidata = mdmalloc (sizeof (atidata_t));
  atidata->python = sysvarsGetStr (SV_PATH_PYTHON);
  atidata->mutagen = sysvarsGetStr (SV_PATH_MUTAGEN);
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;
  atidata->tagName = tagName;
  atidata->audioTagLookup = audioTagLookup;
  atidata->ptre = regexInit ("<PictureType\\.[\\w_]+: (\\d+)>");
  atidata->tagre = regexInit ("': ");
  atidata->encre = regexInit ("<Encoding\\.[\\w_]+: (\\d+)>");

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    regexFree (atidata->ptre);
    regexFree (atidata->tagre);
    regexFree (atidata->encre);
    mdfree (atidata);
  }
}

void
atiiSupportedTypes (int supported [])
{
  /* as of 2023-6-29 */
  /* this needs to be checked */
  for (int i = 0; i < AFILE_TYPE_MAX; ++i) {
    if (i == AFILE_TYPE_UNKNOWN) {
      continue;
    }
    supported [i] = ATI_READ_WRITE;
  }
  supported [AFILE_TYPE_ASF] = ATI_READ;
}

bool
atiiUseReader (void)
{
  return true;
}

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char        * data;
  const char  * targv [10];
  int         targc = 0;
  size_t      retsz;

  targv [targc++] = atidata->python;
  targv [targc++] = "-X";
  targv [targc++] = "utf8";
  targv [targc++] = atidata->mutagen;
  targv [targc++] = ffn;
  targv [targc++] = NULL;

  data = mdmalloc (ATI_TAG_BUFF_SIZE);
  osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, data, ATI_TAG_BUFF_SIZE, &retsz);
  for (size_t i = 0; i < retsz; ++i) {
    if (data [i] == '\0') {
      data [i] = 'X';
    }
  }
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, const char *ffn,
    char *data, int filetype, int tagtype, int *rewrite)
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

  if (data == NULL) {
    return;
  }

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
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "raw: %s", tstr);
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
        slistSetStr (tagdata, atidata->tagName (TAG_DURATION), duration);
      } else {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "no 'seconds' found");
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
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rewrite: various");
        *rewrite |= AF_REWRITE_VARIOUS;
      }

      if (strcmp (p, "TXXX=DURATION") == 0 ||
          strcmp (p, "DURATION") == 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "rewrite: duration");
        *rewrite |= AF_REWRITE_DURATION;
      }

      tagname = atidata->tagLookup (tagtype, p);
      if (tagname != NULL && *tagname != '\0') {
        int   outidx;

        logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "tag: %s raw-tag: %s", tagname, p);

        p = strtok_r (NULL, "=", &tokstrB);
        /* p is pointing to the tag data */
        if (p == NULL) {
          p = "";
        }

        /* mutagen-inspect/pprint() dumps out python code */
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
        /* this stuff always appears in the UFID tag output (binary) */
        if (p != NULL) {
          if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
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
        if (strcmp (tagname, atidata->tagName (TAG_TRACKNUMBER)) == 0) {
          p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
              p, pbuff, sizeof (pbuff));
        }

        /* disc number / disc total handling */
        if (strcmp (tagname, atidata->tagName (TAG_DISCNUMBER)) == 0) {
          p = (char *) atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
              p, pbuff, sizeof (pbuff));
        }

        /* p is pointing to the tag value */

        if (p != NULL && *p != '\0') {
          atioggProcessVorbisComment (atidata->tagLookup, tagdata, tagtype, tagname, p);
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
    int tagtype, int filetype)
{
  int         rc = -1;

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-tags: %s upd:%d del:%d", ffn, slistGetCount (updatelist), slistGetCount (dellist));

  if (tagtype == TAG_TYPE_ID3 && filetype == AFILE_TYPE_MP3) {
    rc = atimutagenWriteMP3Tags (atidata, ffn, updatelist, dellist, datalist);
  } else {
    rc = atimutagenWriteOtherTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }

  return rc;
}

atisaved_t *
atiiSaveTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  atisaved_t  *atisaved;
  char        fn [MAXPATHLEN];
  FILE        *ofh;
  const char  *spacer = "";
  char        *tdata;
  size_t      len;

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (ofh == NULL) {
    return NULL;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "save-tags: %s", ffn);

  tdata = mdmalloc (ATI_TAG_BUFF_SIZE);

  atimutagenWritePythonHeader (atidata, ffn, ofh, tagtype, filetype);
  if (tagtype == TAG_TYPE_ID3) {
    spacer = "  ";
  }
  if (tagtype == TAG_TYPE_ID3) {
    fprintf (ofh, "%stags = audiof.tags\n", spacer);
  } else {
    fprintf (ofh, "%stags = audio.tags\n", spacer);
  }

  fprintf (ofh, "%sprint (tags)\n", spacer);
  atimutagenWritePythonTrailer (ofh, tagtype, filetype);
  mdextfclose (ofh);
  fclose (ofh);

  atisaved = mdmalloc (sizeof (atisaved_t));
  atisaved->hasdata = false;
  atisaved->tagtype = tagtype;
  atisaved->filetype = filetype;

  atimutagenRunUpdate (fn, tdata, ATI_TAG_BUFF_SIZE);
  stringTrim (tdata);
  len = strlen (tdata);
  if (tagtype == TAG_TYPE_ID3) {
    char        *tmp;

    /* the encoding objects seem to cause some syntax errors.  */
    /* don't know why. just fix them. */
    /*    encoding=<Encoding.UTF8: 0> */
    tmp = regexReplace (atidata->encre, tdata, "\\1");
    dataFree (tdata);
    tdata = tmp;
    /* can't do an import * to get all the tag types */
    tmp = regexReplace (atidata->tagre, tdata, "': mutagen.id3.");
    dataFree (tdata);
    tdata = tmp;
    /* these need to be converted also: */
    /*    type=<PictureType.COVER_FRONT: 3> */
    tmp = regexReplace (atidata->ptre, tdata, "\\1");
    dataFree (tdata);
    tdata = tmp;
  }

  atisaved->hasdata = true;
  atisaved->data = tdata;
  atisaved->dlen = len;

  return atisaved;
}

void
atiiFreeSavedTags (atisaved_t *atisaved, int tagtype, int filetype)
{
  if (atisaved == NULL) {
    return;
  }
  if (! atisaved->hasdata) {
    return;
  }
  if (atisaved->tagtype != tagtype) {
    return;
  }
  if (atisaved->filetype != filetype) {
    return;
  }

  atisaved->hasdata = false;
  dataFree (atisaved->data);
  mdfree (atisaved);
}

int
atiiRestoreTags (atidata_t *atidata, atisaved_t *atisaved,
    const char *ffn, int tagtype, int filetype)
{
  char        fn [MAXPATHLEN];
  FILE        *ofh;
  int         rc = -1;
  const char  *spacer = "";

  if (atisaved == NULL) {
    return -1;
  }

  if (! atisaved->hasdata) {
    return -1;
  }

  if (atisaved->tagtype != tagtype) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "restore-tags: mismatched tag type");
    return -1;
  }
  if (atisaved->filetype != filetype) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "restore-tags: mismatched file type");
    return -1;
  }

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (ofh == NULL) {
    return -1;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "restore-tags: %s", ffn);

  atimutagenWritePythonHeader (atidata, ffn, ofh, tagtype, filetype);
  if (tagtype == TAG_TYPE_ID3) {
    spacer = "  ";
  }
  fprintf (ofh, "%saudio.delete()\n", spacer);
  fprintf (ofh, "%stags = %s\n", spacer, atisaved->data);
  if (tagtype == TAG_TYPE_MP4 || tagtype == TAG_TYPE_ID3) {
    fprintf (ofh, "%sfor (k,v) in tags.items():\n", spacer);
  } else {
    fprintf (ofh, "%sfor (k,v) in tags:\n", spacer);
  }
  fprintf (ofh, "%s  audio[k]=v\n", spacer);
  atimutagenWritePythonTrailer (ofh, tagtype, filetype);
  mdextfclose (ofh);
  fclose (ofh);

  rc = atimutagenRunUpdate (fn, NULL, 0);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  return 0;
}

void
atiiCleanTags (atidata_t *atidata,
    const char *ffn, int tagtype, int filetype)
{
  char        fn [MAXPATHLEN];
  FILE        *ofh;
  int         rc = -1;
  const char  *spacer = "";

  logProcBegin (LOG_PROC, "atimutagenCleanTags");

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (ofh == NULL) {
    return;
  }

  atimutagenWritePythonHeader (atidata, ffn, ofh, tagtype, filetype);
  if (tagtype == TAG_TYPE_ID3) {
    spacer = "  ";
  }
  fprintf (ofh, "%saudio.delete()\n", spacer);
  atimutagenWritePythonTrailer (ofh, tagtype, filetype);
  mdextfclose (ofh);
  fclose (ofh);

  rc = atimutagenRunUpdate (fn, NULL, 0);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  logProcEnd (LOG_PROC, "atimutagenCleanTags", "");
  return;
}

/* internal routines */

static int
atimutagenWriteMP3Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist,
    nlist_t *datalist)
{
  char                fn [MAXPATHLEN];
  int                 tagkey;
  slistidx_t          iteridx;
  const char          *tag;
  const char          *value;
  FILE                *ofh;
  int                 rc;
  int                 writetags;
  const tagaudiotag_t *audiotag;

  logProcBegin (LOG_PROC, "atimutagenWriteMP3Tags");
  writetags = atidata->writetags;

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (ofh == NULL) {
    logProcEnd (LOG_PROC, "atimutagenWriteMP3Tags", "open-fail");
    return -1;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-mp3-tags: (%s)", fn);
  atimutagenWritePythonHeader (atidata, ffn, ofh, TAG_TYPE_ID3, AFILE_TYPE_MP3);

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

    tagkey = atidata->tagCheck (writetags, TAG_TYPE_ID3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    audiotag = atidata->audioTagLookup (tagkey, TAG_TYPE_ID3);
    if (audiotag->desc != NULL) {
      fprintf (ofh, "  audio.delall('%s:%s')\n",
          audiotag->base, audiotag->desc);
    } else {
      fprintf (ofh, "  audio.delall('%s')\n", audiotag->tag);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = atidata->tagCheck (writetags, TAG_TYPE_ID3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    audiotag = atidata->audioTagLookup (tagkey, TAG_TYPE_ID3);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write: %s %s",
        audiotag->tag, value);
    if (tagkey == TAG_RECORDING_ID) {
      fprintf (ofh, "  audio.delall('%s:%s')\n", audiotag->base, audiotag->desc);
      fprintf (ofh, "  audio.add(mutagen.id3.%s(encoding=3, owner=u'%s', data=b'%s'))\n",
          audiotag->base, audiotag->desc, value);
    } else if (tagkey == TAG_TRACKNUMBER ||
        tagkey == TAG_DISCNUMBER) {
      if (value != NULL && *value && strcmp (value, "0") != 0) {
        const char  *tot = NULL;

        if (tagkey == TAG_TRACKNUMBER) {
          tot = nlistGetStr (datalist, TAG_TRACKTOTAL);
        }
        if (tagkey == TAG_DISCNUMBER) {
          tot = nlistGetStr (datalist, TAG_DISCTOTAL);
        }
        if (tot != NULL) {
          fprintf (ofh, "  audio.add(mutagen.id3.%s(encoding=3, text=u'%s/%s'))\n",
              audiotag->tag, value, tot);
        } else {
          fprintf (ofh, "  audio.add(mutagen.id3.%s(encoding=3, text=u'%s'))\n",
              audiotag->tag, value);
        }
      }
    } else if (audiotag->desc != NULL) {
      fprintf (ofh, "  audio.add(mutagen.id3.%s(encoding=3, desc=u'%s', text=u'%s'))\n",
          audiotag->base, audiotag->desc, value);
    } else {
      fprintf (ofh, "  audio.add(mutagen.id3.%s(encoding=3, text=u'%s'))\n",
          audiotag->tag, value);
    }
  }

  atimutagenWritePythonTrailer (ofh, TAG_TYPE_ID3, AFILE_TYPE_MP3);
  mdextfclose (ofh);
  fclose (ofh);

  rc = atimutagenRunUpdate (fn, NULL, 0);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  logProcEnd (LOG_PROC, "atimutagenWriteMP3Tags", "");
  return rc;
}

static int
atimutagenWriteOtherTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  char                fn [MAXPATHLEN];
  int                 tagkey;
  slistidx_t          iteridx;
  const char          *tag;
  const char          *value;
  FILE                *ofh;
  int                 rc;
  int                 writetags;
  const tagaudiotag_t *audiotag;

  logProcBegin (LOG_PROC, "atimutagenWriteOtherTags");
  writetags = atidata->writetags;

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  if (ofh == NULL) {
    logProcEnd (LOG_PROC, "atimutagenWriteOtherTags", "open-fail");
    return -1;
  }

  logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "write-other-tags: (%s)", fn);
  atimutagenWritePythonHeader (atidata, ffn, ofh, tagtype, filetype);

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

    tagkey = atidata->tagCheck (writetags, tagtype, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }
    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    fprintf (ofh, "audio.pop('%s')\n", audiotag->tag);
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = atidata->tagCheck (writetags, tagtype, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    audiotag = atidata->audioTagLookup (tagkey, tagtype);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write: %s %s",
        audiotag->tag, value);
    if (tagtype == TAG_TYPE_MP4 &&
        tagkey == TAG_BPM) {
      fprintf (ofh, "audio['%s'] = [%s]\n", audiotag->tag, value);
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
        fprintf (ofh, "audio['%s'] = [(%s,%s)]\n", audiotag->tag, value, tot);
      }
    } else if (tagtype == TAG_TYPE_MP4 && audiotag->base != NULL) {
      fprintf (ofh, "audio['%s'] = bytes ('%s', 'UTF-8')\n",
          audiotag->tag, value);
    } else {
      fprintf (ofh, "audio['%s'] = u'%s'\n", audiotag->tag, value);
    }
  }

  atimutagenWritePythonTrailer (ofh, tagtype, filetype);
  mdextfclose (ofh);
  fclose (ofh);

  rc = atimutagenRunUpdate (fn, NULL, 0);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write failed: %s", ffn);
  }

  logProcEnd (LOG_PROC, "atimutagenWriteOtherTags", "");
  return rc;
}

static void
atimutagenMakeTempFilename (char *fn, size_t sz)
{
  char        tbuff [MAXPATHLEN];

  snprintf (tbuff, sizeof (tbuff), "atimutagen-%" PRId64 ".py", (int64_t) globalCounter++);
  pathbldMakePath (fn, sz, tbuff, "", PATHBLD_MP_DREL_TMP);
}

static int
atimutagenRunUpdate (const char *fn, char *dbuff, size_t sz)
{
  const char  *targv [20];
  int         targc = 0;
  int         rc;

  targv [targc++] = sysvarsGetStr (SV_PATH_PYTHON);
  targv [targc++] = "-X";
  targv [targc++] = "utf8";
  targv [targc++] = fn;
  targv [targc++] = NULL;
  /* the wait flag is on, the return code is the process return code */
  // logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  process (%s)", fn);
  rc = osProcessPipe (targv, OS_PROC_WAIT | OS_PROC_DETACH, dbuff, sz, NULL);
  if (rc == 0) {
    fileopDelete (fn);
  } else {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write tags failed %d (%s)", rc, fn);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  output: %s", dbuff);
  }
  return rc;
}

static void
atimutagenWritePythonHeader (atidata_t *atidata, const char *ffn,
    FILE *ofh, int tagtype, int filetype)
{
  if (tagtype == TAG_TYPE_ID3 && filetype == AFILE_TYPE_MP3) {
    /* include TYER and TDAT in case of an idv2.3 tag */
    fprintf (ofh, "import mutagen\n");
    fprintf (ofh, "from mutagen.mp3 import MP3\n");
    fprintf (ofh, "from mutagen.id3 import ID3,ID3NoHeaderError\n");
    fprintf (ofh, "try:\n");
    /* reads the file twice */
    /* but resolves the no-header situation */
    fprintf (ofh, "  audiof = MP3('%s')\n", ffn);
    fprintf (ofh, "  if audiof.tags is None:\n");
    fprintf (ofh, "    audiof.add_tags()\n");
    fprintf (ofh, "    audiof.save()\n");
    fprintf (ofh, "  audio = ID3('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_FLAC) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: flac");
    fprintf (ofh, "from mutagen.flac import FLAC\n");
    fprintf (ofh, "audio = FLAC('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_MP4) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: mp4");
    fprintf (ofh, "from mutagen.mp4 import MP4,MP4FreeForm,AtomDataType\n");
    fprintf (ofh, "audio = MP4('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: opus");
    fprintf (ofh, "from mutagen.oggopus import OggOpus\n");
    fprintf (ofh, "audio = OggOpus('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_VORBIS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: oggvorbis");
    fprintf (ofh, "from mutagen.oggvorbis import OggVorbis\n");
    fprintf (ofh, "audio = OggVorbis('%s')\n", ffn);
  }
}

static void
atimutagenWritePythonTrailer (FILE *ofh, int tagtype, int filetype)
{
  if (tagtype == TAG_TYPE_ID3 && filetype == AFILE_TYPE_MP3) {
    fprintf (ofh, "  audio.save()\n");
    fprintf (ofh, "except ID3NoHeaderError as e:\n");
    fprintf (ofh, "  exit (1)\n");
  } else {
    fprintf (ofh, "audio.save()\n");
  }
}

