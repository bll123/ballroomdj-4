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

#include <iostream>
#include <string_view>

#include <tagparser/diagnostics.h>
#include <tagparser/mediafileinfo.h>
#include <tagparser/progressfeedback.h>
#include <tagparser/tag.h>
#include <tagparser/tagvalue.h>

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
  return "tagparser";
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

  using namespace std::literals;    // may not need this
  using namespace TagParser;

  writetags = atidata->writetags;

  auto fileInfo = MediaFileInfo ();
  auto diag = Diagnostics ();
  auto progress = AbortableProgressFeedback(
      [](AbortableProgressFeedback &feedback) {
          // callback for status update
      },
      [](AbortableProgressFeedback &feedback) {
          // callback for percentage-only updates
      });
  std::string_view fn (data);
  fileInfo.setPath (fn);
  fileInfo.open ();
  fileInfo.parseContainerFormat(diag, progress);
  fileInfo.parseTags (diag, progress);
//  auto tag = fileInfo.tags ().at (0);
  for (const auto &tag : fileInfo.tags()) {
    auto str = tag->toString ();
    fprintf (stderr, "tagp: %s\n", str.c_str ());
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
