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
#include <assert.h>

#include <audioproperties.h>
#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

extern "C" {

#include "ati.h"
#include "audiofile.h"
#include "mdebug.h"
#include "nlist.h"
#include "slist.h"
#include "tagdef.h"

typedef struct atidata {
  int               writetags;
  taglookup_t       tagLookup;
  tagcheck_t        tagCheck;
  tagname_t         tagName;
  audiotaglookup_t  audioTagLookup;
} atidata_t;

const char *
atiiDesc (void)
{
  /* not supported */
  return NULL;
//  return "taglib";
}

atidata_t *
atiiInit (const char *atipkg, int writetags,
    taglookup_t tagLookup, tagcheck_t tagCheck,
    tagname_t tagName, audiotaglookup_t audioTagLookup)
{
  atidata_t *atidata;

  atidata = (atidata_t *) mdmalloc (sizeof (atidata_t));
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

char *
atiiReadTags (atidata_t *atidata, const char *ffn)
{
  char        * data = NULL;

  data = strdup (ffn);
  return data;
}

void
atiiParseTags (atidata_t *atidata, slist_t *tagdata, char *data,
    int tagtype, int *rewrite)
{
  int         writetags;
  char        pbuff [40];
  const char  *p;

  writetags = atidata->writetags;

  TagLib::FileRef f(data, true, TagLib::AudioProperties::Accurate);

  if (! f.isNull() && f.audioProperties()) {
    long  duration = 0;

    TagLib::AudioProperties *properties = f.audioProperties ();

    duration = properties->lengthInMilliseconds ();
    fprintf (stderr, "taglib: duration: %ld\n", duration);
    slistSetNum (tagdata, atidata->tagName (TAG_DURATION), duration);
  }

  if (! f.isNull() && f.tag()) {
    TagLib::PropertyMap tags = f.file()->properties();

    for (TagLib::PropertyMap::ConstIterator i = tags.begin(); i != tags.end(); ++i) {
      for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
        const char    *tagname = NULL;
        const char    *rawnm = NULL;
        const char    *value = NULL;

        rawnm = i->first.toCString (true);

        if (strcmp (rawnm, "VARIOUSARTISTS") == 0) {
          *rewrite |= AF_REWRITE_VARIOUS;
          continue;
        }

        value = j->toCString (true);
        p = value;
        fprintf (stderr, "taglib: %s %s\n", rawnm, p);

        /* the taglookup() routine would need to be modified to handle */
        /* both vorbis tags and the raw tag name (easy) */
        tagname = atidata->tagLookup (tagtype, rawnm);

        if (tagname != NULL && *tagname != '\0') {
          int   tagkey;

          tagkey = atidata->tagCheck (writetags, tagtype, tagname, AF_REWRITE_NONE);

          /* track number / track total handling */
          if (tagkey == TAG_TRACKNUMBER) {
            p = atiParsePair (tagdata, atidata->tagName (TAG_TRACKTOTAL),
                value, pbuff, sizeof (pbuff));
          }

          /* disc number / disc total handling */
          if (tagkey == TAG_DISCNUMBER) {
            p = atiParsePair (tagdata, atidata->tagName (TAG_DISCTOTAL),
                value, pbuff, sizeof (pbuff));
          }

          slistSetStr (tagdata, tagname, p);
        }
        /* bdj4 can't handle multiple values */
        break;
      }
    }


    /* this appears to be a list of unsupported strings w/o the */
    /* associated data.  not very useful. */
    /* unless taglib starts supporting unsupported tags, I don't see how */
    /* it can be used */
    for (auto s : tags.unsupportedData()) {
      const char    *rawnm = NULL;

      rawnm = s.toCString (true);

      fprintf (stderr, "taglib us: %s\n", rawnm);
    }
  }

  return;
}

int
atiiWriteTags (atidata_t *atidata, const char *ffn,
    slist_t *updatelist, slist_t *dellist, nlist_t *datalist,
    int tagtype, int filetype)
{
  int         rc = -1;

  return rc;
}

} /* extern C */
