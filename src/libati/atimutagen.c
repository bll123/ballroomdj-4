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
#include "audiofile.h"
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
} atidata_t;

static ssize_t globalCounter = 0;

static int  atimutagenWriteMP3Tags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist);
static int  atimutagenWriteOtherTags (atidata_t *atidata, const char *ffn, slist_t *updatelist, slist_t *dellist, nlist_t *datalist, int tagtype, int filetype);
static void atimutagenMakeTempFilename (char *fn, size_t sz);
static int  atimutagenRunUpdate (const char *fn);

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
  atidata->mutagen = sysvarsGetStr (SV_PYTHON_MUTAGEN);
  atidata->writetags = writetags;
  atidata->tagLookup = tagLookup;
  atidata->tagCheck = tagCheck;
  atidata->tagName = tagName;
  atidata->audioTagLookup = audioTagLookup;

  return atidata;
}

void
atiiFree (atidata_t *atidata)
{
  if (atidata != NULL) {
    mdfree (atidata);
  }
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
  const char  * targv [5];
  size_t      retsz;

  targv [0] = atidata->python;
  targv [1] = atidata->mutagen;
  targv [2] = ffn;
  targv [3] = NULL;

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
    int tagtype, int filetype)
{
  int         rc = -1;

  if (tagtype == TAG_TYPE_MP3 && filetype == AFILE_TYPE_MP3) {
    rc = atimutagenWriteMP3Tags (atidata, ffn, updatelist, dellist, datalist);
  } else {
    rc = atimutagenWriteOtherTags (atidata, ffn, updatelist, dellist, datalist, tagtype, filetype);
  }

  return rc;
}

static int
atimutagenWriteMP3Tags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist,
    nlist_t *datalist)
{
  char                fn [MAXPATHLEN];
  int                 tagkey;
  slistidx_t          iteridx;
  char                *tag;
  char                *value;
  FILE                *ofh;
  int                 rc;
  int                 writetags;
  const tagaudiotag_t *audiotag;

  logProcBegin (LOG_PROC, "atimutagenWriteMP3Tags");
  writetags = atidata->writetags;

  atimutagenMakeTempFilename (fn, sizeof (fn));
  ofh = fileopOpen (fn, "w");
  /* include TYER and TDAT in case of an idv2.3 tag */
  fprintf (ofh, "from mutagen.id3 import ID3,TXXX,UFID,TYER,TDAT");
  for (int i = 0; i < TAG_KEY_MAX; ++i) {
    audiotag = atidata->audioTagLookup (i, TAG_TYPE_MP3);
    if (audiotag->tag != NULL && audiotag->desc == NULL) {
      fprintf (ofh, ",%s", audiotag->tag);
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

    tagkey = atidata->tagCheck (writetags, TAG_TYPE_MP3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    audiotag = atidata->audioTagLookup (tagkey, TAG_TYPE_MP3);
    if (audiotag->desc != NULL) {
      fprintf (ofh, "  audio.delall('%s:%s')\n",
          audiotag->base, audiotag->desc);
    } else {
      fprintf (ofh, "  audio.delall('%s')\n", audiotag->tag);
    }
  }

  slistStartIterator (updatelist, &iteridx);
  while ((tag = slistIterateKey (updatelist, &iteridx)) != NULL) {
    tagkey = atidata->tagCheck (writetags, TAG_TYPE_MP3, tag, AF_REWRITE_NONE);
    if (tagkey < 0) {
      continue;
    }

    value = slistGetStr (updatelist, tag);

    audiotag = atidata->audioTagLookup (tagkey, TAG_TYPE_MP3);
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "  write: %s %s",
        audiotag->tag, value);
    if (tagkey == TAG_RECORDING_ID) {
      fprintf (ofh, "  audio.delall('%s:%s')\n", audiotag->base, audiotag->desc);
      fprintf (ofh, "  audio.add(%s(encoding=3, owner=u'%s', data=b'%s'))\n",
          audiotag->base, audiotag->desc, value);
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
              audiotag->tag, value, tot);
        } else {
          fprintf (ofh, "  audio.add(%s(encoding=3, text=u'%s'))\n",
              audiotag->tag, value);
        }
      }
    } else if (audiotag->desc != NULL) {
      fprintf (ofh, "  audio.add(%s(encoding=3, desc=u'%s', text=u'%s'))\n",
          audiotag->base, audiotag->desc, value);
    } else {
      fprintf (ofh, "  audio.add(%s(encoding=3, text=u'%s'))\n",
          audiotag->tag, value);
    }
  }

  fprintf (ofh, "  audio.save()\n");
  fprintf (ofh, "except ID3NoHeaderError as e:\n");
  fprintf (ofh, "  exit (1)\n");
  fclose (ofh);

  rc = atimutagenRunUpdate (fn);
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
  char                *tag;
  char                *value;
  FILE                *ofh;
  int                 rc;
  int                 writetags;
  const tagaudiotag_t *audiotag;

  logProcBegin (LOG_PROC, "atimutagenWriteOtherTags");
  writetags = atidata->writetags;

  atimutagenMakeTempFilename (fn, sizeof (fn));
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
  if (filetype == AFILE_TYPE_OPUS) {
    logMsg (LOG_DBG, LOG_DBUPDATE | LOG_AUDIO_TAG, "file-type: opus");
    fprintf (ofh, "from mutagen.oggopus import OggOpus\n");
    fprintf (ofh, "audio = OggOpus('%s')\n", ffn);
  }
  if (filetype == AFILE_TYPE_OGG) {
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

  fprintf (ofh, "audio.save()\n");
  fclose (ofh);

  rc = atimutagenRunUpdate (fn);
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
atimutagenRunUpdate (const char *fn)
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

