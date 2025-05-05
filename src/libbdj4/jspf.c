/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <json-c/json.h>

#include "audiosrc.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "expimp.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
#include "pathutil.h"
#include "song.h"
#include "tagdef.h"

void
jspfExport (musicdb_t *musicdb, nlist_t *list,
    const char *fname, const char *slname)
{
  FILE        *fh;
  nlistidx_t  iteridx;
  dbidx_t     dbidx;
  song_t      *song;
  const char  *str;
  char        ffn [MAXPATHLEN];
  bool        first = true;

  fh = fileopOpen (fname, "w");
  if (fh == NULL) {
    return;
  }

  fprintf (fh, "{\n");
  fprintf (fh, "  \"playlist\" : {\n");
  fprintf (fh, "    \"title\" : \"%s\",\n", slname);
  fprintf (fh, "    \"track\" : [\n");

  nlistStartIterator (list, &iteridx);
  while ((dbidx = nlistIterateKey (list, &iteridx)) >= 0) {
    const char    *pfx;

    song = dbGetByIdx (musicdb, dbidx);

    if (song == NULL) {
      continue;
    }

    pfx = audiosrcPrefix (songGetStr (song, TAG_URI));

    if (first) {
      first = false;
    } else {
      fprintf (fh, ",\n");
    }
    fprintf (fh, "    {\n");

    str = songGetStr (song, TAG_URI);
    audiosrcFullPath (str, ffn, sizeof (ffn), NULL, 0);
    pathDisplayPath (ffn, strlen (ffn));
    fprintf (fh, "      \"location\" : [\"%s%s\"],\n", pfx, ffn);

    fprintf (fh, "      \"title\" : \"%s\",\n", songGetStr (song, TAG_TITLE));

    str = songGetStr (song, TAG_ARTIST);
    if (str != NULL && *str) {
      fprintf (fh, "      \"creator\" : \"%s\",\n", str);
    }

    str = songGetStr (song, TAG_ALBUM);
    if (str != NULL && *str) {
      fprintf (fh, "      \"album\" : \"%s\",\n", str);
    }

    /* no trailing comma */
    fprintf (fh, "      \"duration\" : %" PRId64 "\n",
        songGetNum (song, TAG_DURATION));

    /* comma and newline handled elsewhere */
    fprintf (fh, "    }");
  }

  fprintf (fh, "\n");
  fprintf (fh, "    ]\n");
  fprintf (fh, "  }\n");
  fprintf (fh, "}\n");

  mdextfclose (fh);
  fclose (fh);
}

nlist_t *
jspfImport (musicdb_t *musicdb, const char *fname)
{
  json_object   *jroot;
  json_tokener  *jtok;
  json_object   *jpl;
  json_object   *jtmp;
  json_object   *jtrklist;
  int           jerr;
  char          *data;
  size_t        len;
  nlist_t       *list = NULL;
  int           trkcount;
  const char    *val;
  char          tbuff [MAXPATHLEN];
  song_t        *song;
  dbidx_t       dbidx;

  data = filedataReadAll (fname, &len);

  list = nlistAlloc ("jspfimport", LIST_UNORDERED, NULL);

  jtok = json_tokener_new ();
  jroot = json_tokener_parse_ex (jtok, data, len);
  jerr = json_tokener_get_error (jtok);
  if (jerr != json_tokener_success) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "parse: failed: %d / %s\n", jerr,
        json_tokener_error_desc (jerr));
    dataFree (data);
    return NULL;
  }

  jpl = json_object_object_get (jroot, "playlist");
  if (jpl == NULL) {
    dataFree (data);
    return NULL;
  }

  jtmp = json_object_object_get (jpl, "title");
  if (jtmp == NULL) {
    dataFree (data);
    return NULL;
  }
  /* the playlist name is not used */
#if 0
  val = json_object_get_string (jtmp);
  if (val != NULL) {
    stpecpy (plname, plname + plsz, val);
  }
#endif

  jtrklist = json_object_object_get (jpl, "track");
  if (jtrklist == NULL) {
    dataFree (data);
    return NULL;
  }

  trkcount = json_object_array_length (jtrklist);
  for (int i = 0; i < trkcount; ++i) {
    json_object   *jtrk;

    jtrk = json_object_array_get_idx (jtrklist, i);
    if (jtrk == NULL) {
      continue;
    }

    jtmp = json_object_object_get (jtrk, "location");
    if (jtmp == NULL) {
      continue;
    }
    jtmp = json_object_array_get_idx (jtmp, 0);
    if (jtrk == NULL) {
      continue;
    }

    val = json_object_get_string (jtmp);
    if (val == NULL) {
      continue;
    }

    stpecpy (tbuff, tbuff + sizeof (tbuff), val);
    pathNormalizePath (tbuff, strlen (tbuff));
    val = audiosrcRelativePath (tbuff, 0);
    song = dbGetByName (musicdb, val);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (list, dbidx, 0);
      }
    }
  }

  json_tokener_free (jtok);
  json_object_put (jroot);
  dataFree (data);

  return list;
}
