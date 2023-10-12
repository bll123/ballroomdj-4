/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 *
 * https://docs.acrcloud.com/reference/identification-api
 * metadata info (not very good):
 *   https://docs.acrcloud.com/metadata/music
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

#include <glib.h>
#include <gcrypt.h>

#include "audioid.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "fileop.h"
#include "ilist.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "pathbld.h"
#include "song.h"
#include "songutil.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

typedef struct audioidacr {
  const char    *key;
  const char    *secret;
  webclient_t   *webclient;
  mstime_t      globalreqtimer;
  int           globalreqcount;
  const char    *webresponse;
  size_t        secretlen;
  size_t        webresplen;
  int           respcount;
} audioidacr_t;

enum {
  FREE_LIMIT = 100,
  QPS_LIMIT = 1000 / 2 + 1,
};

/* roles: AssociatedPerformer, Composer, Conductor, MainArtist */
static audioidparsedata_t acrroles [] = {
  { TAG_ARTIST, "AssociatedPerformer" },
  { TAG_CONDUCTOR, "Conductor" },
  { TAG_COMPOSER, "Composer" },
  { TAG_ALBUMARTIST, "MainArtist" },
  { -1, NULL },
};

static audioidparse_t acrartistsjp [] = {
  { AUDIOID_PARSE_DATA,   TAG_ARTIST, "name", NULL, NULL, NULL },
  /* must appear after tag_artist */
  { AUDIOID_PARSE_DATA_ARRAY, TAG_ARTIST, "roles", NULL, NULL, acrroles },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_ARRAY, "end-artists", NULL, NULL, NULL },
};

static audioidparse_t acralbumjp [] = {
  { AUDIOID_PARSE_DATA,   TAG_ALBUM, "name", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-album", NULL, NULL, NULL },
};

static audioidparse_t acrmusicjp [] = {
  { AUDIOID_PARSE_DATA,   TAG_TITLE, "title", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA,   TAG_AUDIOID_SCORE, "score", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA,   TAG_DURATION, "duration_ms", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA,   TAG_DATE, "release_date", NULL, NULL, NULL },
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "album", NULL, acralbumjp, NULL },
  { AUDIOID_PARSE_ARRAY,  AUDIOID_TYPE_ARRAY, "artists", NULL, acrartistsjp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_ARRAY, "end-music", NULL, NULL, NULL },
};

static audioidparse_t acrmetadatajp [] = {
  { AUDIOID_PARSE_ARRAY,  AUDIOID_TYPE_RESPIDX, "music", NULL, acrmusicjp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-metadata", NULL, NULL, NULL },
};

static audioidparse_t acrstatusjp [] = {
  { AUDIOID_PARSE_DATA,   AUDIOID_TYPE_STATUS_CODE, "code", NULL, NULL, NULL },
  { AUDIOID_PARSE_DATA,   AUDIOID_TYPE_STATUS_MSG, "msg", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-status", NULL, NULL, NULL },
};

static audioidparse_t acrmainjp [] = {
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "metadata", NULL, acrmetadatajp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-response", NULL, NULL, NULL },
};

static audioidparse_t acrmainstatusjp [] = {
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "status", NULL, acrstatusjp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-response", NULL, NULL, NULL },
};

static void acrWebResponseCallback (void *userdata, const char *resp, size_t len);
static void dumpData (audioidacr_t *acr);

audioidacr_t *
acrInit (void)
{
  audioidacr_t    *acr;
  const char      *tver;

  acr = mdmalloc (sizeof (audioidacr_t));
  acr->webclient = webclientAlloc (acr, acrWebResponseCallback);
  acr->webresponse = NULL;
  acr->webresplen = 0;
  mstimeset (&acr->globalreqtimer, 0);
  acr->globalreqcount = 0;
  acr->key = bdjoptGetStr (OPT_G_ACRCLOUD_API_KEY);
  acr->secret = bdjoptGetStr (OPT_G_ACRCLOUD_API_SECRET);
  acr->secretlen = strlen (acr->secret);

  /* a call to gcry_check_version is required to initialize */
  /* the gcrypt library */
  tver = gcry_check_version (NULL);
  logMsg (LOG_DBG, LOG_AUDIO_ID, "gcrypt version: %s", tver);

  return acr;
}

void
acrFree (audioidacr_t *acr)
{
  if (acr == NULL) {
    return;
  }

  webclientClose (acr->webclient);
  mdfree (acr);
}

int
acrLookup (audioidacr_t *acr, const song_t *song, ilist_t *respdata)
{
  char            infn [MAXPATHLEN];
  char            uri [MAXPATHLEN];
  char            sig [MAXPATHLEN];
  unsigned char   digest [200];
  size_t          rdlen;
  time_t          tm;
  char            *b64sig;
  gcry_mac_hd_t   gch;
  size_t          fpsize;
  char            fpszstr [40];
  char            ts [40];
  const char      *query [40];
  int             qc = 0;
  const char      *targv [15];
  int             targc = 0;
  const char      *fn;
  char            *ffn;
  char            fpfn [MAXPATHLEN];
  mstime_t        starttm;
  int             webrc;
  int             rc;
  const char      *tstr;
  int             respidx;

  if (acr->key == NULL || ! *acr->key ||
      acr->secret == NULL || ! *acr->secret) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: not configured");
    return 0;
  }

  ++acr->globalreqcount;
  if (acr->globalreqcount > FREE_LIMIT) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: request limit reached");
    return 0;
  }

  mstimestart (&starttm);
  /* acrcloud allows at most two calls per second for the free service */
  while (! mstimeCheck (&acr->globalreqtimer)) {
    mssleep (10);
  }
  mstimeset (&acr->globalreqtimer, QPS_LIMIT);
  logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: wait time: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  tm = time (NULL);

  fn = songGetStr (song, TAG_FILE);
  ffn = songutilFullFileName (fn);
  if (! fileopFileExists (ffn)) {
    return 0;
  }
  snprintf (infn, sizeof (infn), "%s%s",
      ffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
  /* check for .original filename */
  if (! fileopFileExists (infn)) {
    strlcpy (infn, ffn, sizeof (infn));
  }
  mdfree (ffn);

  pathbldMakePath (fpfn, sizeof (fpfn), "acrcloud-fp", BDJ4_CONFIG_EXT,
      PATHBLD_MP_DREL_TMP);

  targv [targc++] = sysvarsGetStr (SV_PATH_ACRCLOUD);
  targv [targc++] = "-i";
  targv [targc++] = infn;
  targv [targc++] = "-cli";
  targv [targc++] = "-o";
  targv [targc++] = fpfn;
  targv [targc++] = NULL;
  mstimestart (&starttm);
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: fp: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  snprintf (uri, sizeof (uri), "https://%s/v1/identify",
      bdjoptGetStr (OPT_G_ACRCLOUD_API_HOST));

  /* no newline at the end */
  snprintf (sig, sizeof (sig), "POST\n/v1/identify\n%s\nfingerprint\n1\n%ld",
      acr->key, tm);
  gch = NULL;
  rc = gcry_mac_open (&gch, GCRY_MAC_HMAC_SHA1, 0, NULL);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mac-open error: %s", gcry_strerror (rc));
    return 0;
  }
  rc = gcry_mac_setkey (gch, acr->secret, acr->secretlen);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mac-setkey error: %s", gcry_strerror (rc));
    return 0;
  }
  rc = gcry_mac_write (gch, sig, strlen (sig));
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mac-write error: %s", gcry_strerror (rc));
    return 0;
  }
  *digest = '\0';
  rdlen = sizeof (digest);
  rc = gcry_mac_read (gch, digest, &rdlen);
  if (rc != GPG_ERR_NO_ERROR) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "mac-read error: %s", gcry_strerror (rc));
    return 0;
  }
  b64sig = g_base64_encode (digest, rdlen);

  fpsize = fileopSize (fpfn);
  snprintf (fpszstr, sizeof (fpszstr), "%ld", (long) fpsize);
  snprintf (ts, sizeof (ts), "%ld", tm);

  query [qc++] = "sample_bytes";
  query [qc++] = fpszstr;
  query [qc++] = "access_key";
  query [qc++] = acr->key;
  query [qc++] = "data_type";
  query [qc++] = "fingerprint";
  query [qc++] = "signature";
  query [qc++] = b64sig;
  query [qc++] = "signature_version";
  query [qc++] = "1";
  query [qc++] = "timestamp";
  query [qc++] = ts;
  query [qc++] = NULL;

  mstimestart (&starttm);
  webrc = webclientUploadFile (acr->webclient, uri, query, fpfn, "sample");
  logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: web-query: %d %" PRId64 "ms",
      webrc, (int64_t) mstimeend (&starttm));
  if (webrc != WEB_OK) {
    return 0;
  }

  gcry_mac_close (gch);
  dataFree (b64sig);

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpData (acr);
  }

  mstimestart (&starttm);
  audioidParseJSONAll (acr->webresponse, acr->webresplen,
      acrmainstatusjp, respdata, AUDIOID_ID_ACRCLOUD);
  respidx = ilistGetNum (respdata, 0, AUDIOID_TYPE_RESPIDX);
  tstr = ilistGetStr (respdata, respidx, AUDIOID_TYPE_STATUS_CODE);
  rc = -1;
  if (tstr != NULL) {
    rc = atoi (tstr);
  }
  if (rc == 0) {
    acr->respcount = audioidParseJSONAll (acr->webresponse, acr->webresplen,
        acrmainjp, respdata, AUDIOID_ID_ACRCLOUD);
  } else if (rc == -1) {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "acrcloud: code: %d / %s", rc,
        nlistGetStr (respdata, AUDIOID_TYPE_STATUS_MSG));
  } else {
    logMsg (LOG_DBG, LOG_AUDIO_ID, "acrcloud: parse failed");
  }
  logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: parse: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  return acr->respcount;
}

static void
acrWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidacr_t *acr = userdata;

  acr->webresponse = resp;
  acr->webresplen = len;
  return;
}

static void
dumpData (audioidacr_t *acr)
{
  FILE *ofh;

  if (acr->webresponse != NULL && acr->webresplen > 0) {
    ofh = fopen ("out-acr.json", "w");
    if (ofh != NULL) {
      fwrite (acr->webresponse, 1, acr->webresplen, ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}

