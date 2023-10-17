/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 *
 * Parses JSON responses
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>

#include <json-c/json.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjstring.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "tagdef.h"

static bool audioidParse (json_object *jtop, audioidparse_t *jsonp, int jidx, ilist_t *respdata, int level, audioid_id_t ident);
static bool audioidParseTree (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident);
static bool audioidParseArray (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident);
static bool audioidParseDataArray (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, audioidparsedata_t *jdata, ilist_t *respdata, int level, audioid_id_t ident);
static void dumpDataStr (const char *str);

int
audioidParseJSONAll (const char *data, size_t datalen,
    audioidparse_t *jsonp, ilist_t *respdata, audioid_id_t ident)
{
  json_object   *jroot;
  json_tokener  *jtok;
  int           jerr;
  int           respcount = 0;
  int           respidx;

  jtok = json_tokener_new ();
  jroot = json_tokener_parse_ex (jtok, data, datalen);
  jerr = json_tokener_get_error (jtok);
  if (jerr != json_tokener_success) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "parse: failed: %d / %s\n", jerr,
        json_tokener_error_desc (jerr));
    return 0;
  }

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    const char  *tval;

    tval = json_object_to_json_string_ext (jroot,
        JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_NOSLASHESCAPE |
        JSON_C_TO_STRING_SPACED);
    dumpDataStr (tval);
  }

  /* beginning response index */
  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);

  audioidParse (jroot, jsonp, 0, respdata, 0, ident);

  respcount = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX) - respidx;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "json-parse: respcount: %d", respcount);

  json_tokener_free (jtok);
  json_object_put (jroot);

  return respcount;
}

static bool
audioidParse (json_object *jtop, audioidparse_t *jsonp,
    int jidx, ilist_t *respdata, int level, audioid_id_t ident)
{
  int                 respidx;

  if (jtop == NULL) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s jidx: %d %s null json object", level*2, "", jidx, jsonp [jidx].name);
    return false;
  }

  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s jidx: %d %s respidx %d", level*2, "", jidx, jsonp [jidx].name, respidx);

  while (jsonp [jidx].flag != AUDIOID_PARSE_END) {
    json_object   *jtmp;
    int           ttagidx;
    const char    *val;
    const char    *tval = NULL;
    const char    *joinphrase = NULL;


    if (jsonp [jidx].flag == AUDIOID_PARSE_SET) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set: jidx: %d %s", level*2, "", jidx, jsonp [jidx].name);
      if (jsonp [jidx].tagidx == AUDIOID_TYPE_JOINPHRASE) {
        ilistSetStr (respdata, 0, jsonp [jidx].tagidx, jsonp [jidx].name);
      }
      ++jidx;
      continue;
    }

    jtmp = json_object_object_get (jtop, jsonp [jidx].name);
    if (jtmp == NULL)  {
      logMsg (LOG_DBG, LOG_IMPORTANT, "%*s json-parse: %s not found", level*2, "", jsonp [jidx].name);
      ++jidx;
      continue;
    }

    if (jsonp [jidx].flag == AUDIOID_PARSE_TREE) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: jidx: %d %s", level*2, "", jidx, jsonp [jidx].name);
      audioidParseTree (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, respdata, level, ident);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", jsonp [jidx].name);
      ++jidx;
      continue;
    }

    if (jsonp [jidx].flag == AUDIOID_PARSE_ARRAY) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: jidx: %d %s", level*2, "", jidx, jsonp [jidx].name);
      audioidParseArray (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, respdata, level, ident);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-array %s", level*2, "", jsonp [jidx].name);
      ++jidx;
      continue;
    }

    if (jsonp [jidx].flag == AUDIOID_PARSE_DATA_ARRAY) {
      audioidParseDataArray (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, jsonp [jidx].jdata, respdata, level, ident);
      ++jidx;
      continue;
    }

    val = json_object_get_string (jtmp);
    if (val == NULL) {
      ++jidx;
      continue;
    }

    ttagidx = jsonp [jidx].tagidx;

    joinphrase = ilistGetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE);

    tval = (const char *) val;
    if (ttagidx == TAG_AUDIOID_SCORE) {
      /* acrcloud returns a score between 70 and 100 */
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set respidx: %d tagidx: %d %s %s", level*2, "", respidx, ttagidx, tagdefs [ttagidx].tag, tval);
      ilistSetDouble (respdata, respidx, ttagidx, atof (tval));
    } else {
      if (audioidSetResponseData (level, respdata, respidx, ttagidx, tval, joinphrase)) {
        ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, NULL);
      }
    }

    ++jidx;
  }

  return true;
}

static bool
audioidParseTree (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident)
{
  bool    rc;

  rc = audioidParse (jtop, jsonp, 0, respdata, level + 1, ident);

  /* increment the response index after the parse is done */
  if (rc && parenttagidx == AUDIOID_TYPE_RESPIDX) {
    int     respidx;

    respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
    ilistSetNum (respdata, respidx, AUDIOID_TYPE_IDENT, ident);
    ++respidx;
    ilistSetNum (respdata, 0, AUDIOID_TYPE_RESPIDX, respidx);
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s tree: set respidx: %d", level*2, "", respidx);
  }

  return rc;
}

static bool
audioidParseArray (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, ilist_t *respdata, int level, audioid_id_t ident)
{
  int   jidx = 0;
  int   respidx;
  int   mcount;
  int   rc = true;

  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);

  mcount = json_object_array_length (jtop);
  for (int i = 0; i < mcount; ++i)  {
    json_object   *jtmp;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: idx: %d", level*2, "", i);

    jtmp = json_object_array_get_idx (jtop, i);
    if (! audioidParse (jtmp, jsonp, jidx, respdata, level + 1, ident)) {
      rc = false;
    }

    /* increment the response index after the parse is done */
    if (rc && parenttagidx == AUDIOID_TYPE_RESPIDX) {
      respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
      ilistSetNum (respdata, respidx, AUDIOID_TYPE_IDENT, ident);
      ++respidx;
      ilistSetNum (respdata, 0, AUDIOID_TYPE_RESPIDX, respidx);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: set respidx: %d", level*2, "", respidx);
    }
  }

  ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, NULL);

  return true;
}

static bool
audioidParseDataArray (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, audioidparsedata_t *jdata,
    ilist_t *respdata, int level, audioid_id_t ident)
{
  int         respidx;
  int         mcount;
  const char  *joinphrase;

  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
  joinphrase = ilistGetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE);

  mcount = json_object_array_length (jtop);
  for (int i = 0; i < mcount; ++i)  {
    json_object   *jtmp;
    const char    *val;
    int           ttagidx;
    int           jdataidx;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: idx: %d", level*2, "", i);

    jtmp = json_object_array_get_idx (jtop, i);
    if (jtmp == NULL) {
      continue;
    }
    val = json_object_get_string (jtmp);
    if (val == NULL) {
      continue;
    }

    jdataidx = 0;
    while (jdata [jdataidx].name != NULL) {
      if (strcmp (val, jdata [jdataidx].name) == 0) {
        ttagidx = jdata [jdataidx].tagidx;
        val = ilistGetStr (respdata, respidx, parenttagidx);
        if (audioidSetResponseData (level, respdata, respidx, ttagidx, val, joinphrase)) {
          ilistSetStr (respdata, 0, AUDIOID_TYPE_JOINPHRASE, NULL);
        }
        break;
      }
      ++jdataidx;
    }
  }

  return true;
}

static void
dumpDataStr (const char *str)
{
  FILE *ofh;

  if (str != NULL) {
    ofh = fopen ("out-acr.json", "w");
    if (ofh != NULL) {
      fwrite (str, 1, strlen (str), ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}

