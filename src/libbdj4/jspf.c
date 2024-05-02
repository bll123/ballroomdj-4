/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
//#include "bdjopt.h"
#include "bdjstring.h"
#include "filedata.h"
#include "fileop.h"
#include "expimp.h"
#include "log.h"
#include "mdebug.h"
//#include "musicdb.h"
#include "nlist.h"
#include "pathdisp.h"
//#include "pathutil.h"
#include "song.h"
#include "tagdef.h"
//#include "sysvars.h"

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

    pfx = "";
    if (audiosrcGetType (songGetStr (song, TAG_URI)) == AUDIOSRC_TYPE_FILE) {
      pfx = "file://";
    }

    if (first) {
      first = false;
    } else {
      fprintf (fh, ",\n");
    }
    fprintf (fh, "    {\n");

    str = songGetStr (song, TAG_URI);
    audiosrcFullPath (str, ffn, sizeof (ffn), 0, NULL);
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
jspfImport (musicdb_t *musicdb, const char *fname, char *plname, size_t plsz)
{
  json_object   *jroot;
  json_tokener  *jtok;
  int           jerr;
  char          *data;
  size_t        len;
  nlist_t     *list;

  const char  *p;
  song_t      *song;
  dbidx_t     dbidx;
  char        tbuff [MAXPATHLEN];

  data = filedataReadAll (fname, &len);

  list = nlistAlloc ("jspfimport", LIST_UNORDERED, NULL);

  jtok = json_tokener_new ();
  jroot = json_tokener_parse_ex (jtok, data, len);
  jerr = json_tokener_get_error (jtok);
  if (jerr != json_tokener_success) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "parse: failed: %d / %s\n", jerr,
        json_tokener_error_desc (jerr));
    return 0;
  }

  if (logCheck (LOG_DBG, LOG_INFO)) {
    const char  *tval;

    tval = json_object_to_json_string_ext (jroot,
        JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE |
        JSON_C_TO_STRING_SPACED);
//    dumpDataStr (tval);
  }

#if 0
  while (fgets (tbuff, sizeof (tbuff), fh) != NULL) {
    if (strncmp (tbuff, "#PLAYLIST:", 10) == 0) {
      stringTrim (tbuff);
      p = tbuff + 10;
      while (*p == ' ' && *p != '\0') {
        ++p;
      }
      if (*p) {
        strlcpy (plname, p, plsz);
      }
      continue;
    }

    if (*tbuff == '#') {
      continue;
    }

    stringTrim (tbuff);

    pathNormalizePath (tbuff, strlen (tbuff));
    if (! fileopFileExists (tbuff)) {
      continue;
    }

    p = audiosrcRelativePath (tbuff, 0);

    song = dbGetByName (musicdb, p);
    if (song != NULL) {
      dbidx = songGetNum (song, TAG_DBIDX);
      if (dbidx >= 0) {
        nlistSetNum (list, dbidx, 0);
      }
    }
  }
#endif

  json_tokener_free (jtok);
  json_object_put (jroot);

  return list;
}
