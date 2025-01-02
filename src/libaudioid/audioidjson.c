/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
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
#include "log.h"
#include "mdebug.h"
#include "nlist.h"
#include "tagdef.h"

static bool audioidParse (json_object *jtop, audioidparse_t *jsonp, int jidx, audioid_resp_t *resp, int level, audioid_id_t ident);
static bool audioidParseTree (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident);
static bool audioidParseArray (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident);
static bool audioidParseDataArray (json_object *jtop, audioidparse_t *jsonp, int parenttagidx, audioidparsedata_t *jdata, audioid_resp_t *resp, int level, audioid_id_t ident);
static void dumpDataStr (const char *str);

int
audioidParseJSONAll (const char *data, size_t datalen,
    audioidparse_t *jsonp, audioid_resp_t *resp, audioid_id_t ident)
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
  respidx = resp->respidx;
  audioidParse (jroot, jsonp, 0, resp, 0, ident);

  respcount = respidx - resp->respidx + 1;
  logMsg (LOG_DBG, LOG_AUDIO_ID, "json-parse: respcount: %d", respcount);

  json_tokener_free (jtok);
  json_object_put (jroot);

  return respcount;
}

static bool
audioidParse (json_object *jtop, audioidparse_t *jsonp,
    int jidx, audioid_resp_t *resp, int level, audioid_id_t ident)
{
  nlist_t *respdata;

  if (jtop == NULL) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s jidx: %d %s null json object", level*2, "", jidx, jsonp [jidx].name);
    return false;
  }

  respdata = audioidGetResponseData (resp, resp->respidx);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s jidx: %d %s respidx %d", level*2, "", jidx, jsonp [jidx].name, resp->respidx);

  while (jsonp [jidx].flag != AUDIOID_PARSE_END) {
    json_object   *jtmp;
    int           ttagidx;
    const char    *val;
    const char    *tval = NULL;

    if (jsonp [jidx].flag == AUDIOID_PARSE_SET) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set: jidx: %d %s", level*2, "", jidx, jsonp [jidx].name);
      if (jsonp [jidx].tagidx == AUDIOID_TYPE_JOINPHRASE) {
        dataFree (resp->joinphrase);
        resp->joinphrase = mdstrdup (jsonp [jidx].name);
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
      audioidParseTree (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, resp, level, ident);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-tree %s", level*2, "", jsonp [jidx].name);
      ++jidx;
      continue;
    }

    if (jsonp [jidx].flag == AUDIOID_PARSE_ARRAY) {
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: jidx: %d %s", level*2, "", jidx, jsonp [jidx].name);
      audioidParseArray (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, resp, level, ident);
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s finish-array %s", level*2, "", jsonp [jidx].name);
      ++jidx;
      continue;
    }

    if (jsonp [jidx].flag == AUDIOID_PARSE_DATA_ARRAY) {
      audioidParseDataArray (jtmp, jsonp [jidx].tree, jsonp [jidx].tagidx, jsonp [jidx].jdata, resp, level, ident);
      ++jidx;
      continue;
    }

    val = json_object_get_string (jtmp);
    if (val == NULL) {
      ++jidx;
      continue;
    }

    ttagidx = jsonp [jidx].tagidx;

    tval = (const char *) val;
    if (ttagidx == TAG_AUDIOID_SCORE) {
      /* acrcloud returns a score between 70 and 100 */
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s set score(%d): tagidx: %d %s %s", level*2, "", resp->respidx, ttagidx, tagdefs [ttagidx].tag, tval);
      nlistSetDouble (respdata, ttagidx, atof (tval));
    } else {
      /* set-response-data will clear the joinphrase if used */
      audioidSetResponseData (level, resp, ttagidx, tval);
    }

    ++jidx;
  }

  return true;
}

static bool
audioidParseTree (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident)
{
  bool    rc;

  rc = audioidParse (jtop, jsonp, 0, resp, level + 1, ident);

  /* increment the response index after the parse is done */
  if (rc && parenttagidx == AUDIOID_TYPE_TOP) {
    nlist_t *respdata;

    respdata = audioidGetResponseData (resp, resp->respidx);
    nlistSetNum (respdata, TAG_AUDIOID_IDENT, ident);
    resp->respidx += 1;
    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s -- parse-done: tree: new respidx: %d", level*2, "", resp->respidx);
  }

  return rc;
}

static bool
audioidParseArray (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, audioid_resp_t *resp, int level, audioid_id_t ident)
{
  int   jidx = 0;
  int   mcount;
  int   rc = true;

  mcount = json_object_array_length (jtop);
  for (int i = 0; i < mcount; ++i)  {
    json_object   *jtmp;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: idx: %d", level*2, "", i);

    jtmp = json_object_array_get_idx (jtop, i);
    if (! audioidParse (jtmp, jsonp, jidx, resp, level + 1, ident)) {
      rc = false;
    }

    /* increment the response index after the parse is done */
    if (rc && parenttagidx == AUDIOID_TYPE_TOP) {
      nlist_t   *respdata;

      respdata = audioidGetResponseData (resp, resp->respidx);
      nlistSetNum (respdata, TAG_AUDIOID_IDENT, ident);
      resp->respidx += 1;
      logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s -- parse-done: array: new respidx %d", level*2, "", resp->respidx);
    }
  }

  dataFree (resp->joinphrase);
  resp->joinphrase = NULL;

  return true;
}

static bool
audioidParseDataArray (json_object *jtop, audioidparse_t *jsonp,
    int parenttagidx, audioidparsedata_t *jdata,
    audioid_resp_t *resp, int level, audioid_id_t ident)
{
  int         mcount;

  mcount = json_object_array_length (jtop);
  for (int i = 0; i < mcount; ++i)  {
    json_object   *jtmp;
    const char    *val;
    int           ttagidx;
    int           jdataidx;
    nlist_t       *respdata;

    logMsg (LOG_DBG, LOG_AUDIO_ID, "%*s array: idx: %d", level*2, "", i);

    jtmp = json_object_array_get_idx (jtop, i);
    if (jtmp == NULL) {
      continue;
    }
    val = json_object_get_string (jtmp);
    if (val == NULL) {
      continue;
    }

    respdata = audioidGetResponseData (resp, resp->respidx);

    jdataidx = 0;
    while (jdata [jdataidx].name != NULL) {
      if (strcmp (val, jdata [jdataidx].name) == 0) {
        ttagidx = jdata [jdataidx].tagidx;
        val = nlistGetStr (respdata, parenttagidx);
        /* set-response-data will clear the joinphrase if used */
        audioidSetResponseData (level, resp, ttagidx, val);
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
    ofh = fopen (ACRCLOUD_TEMP_FN, "w");
    if (ofh != NULL) {
      fwrite (str, 1, strlen (str), ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}

