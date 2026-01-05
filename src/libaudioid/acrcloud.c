/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
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
#include "audiosrc.h"
#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "fileop.h"
#include "log.h"
#include "mdebug.h"
#include "osprocess.h"
#include "pathbld.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "vsencdec.h"
#include "webclient.h"

typedef struct audioidacr {
  char          key [50];
  char          secret [50];
  webclient_t   *webclient;
  mstime_t      globalreqtimer;
  int           globalreqcount;
  const char    *webresponse;
  size_t        secretlen;
  size_t        webresplen;
  int           respcount;
} audioidacr_t;

/* for debugging only */
/* this is useful, as the free ACRCloud only allows 100 queries/month */
/* note that a valid out-acr.json file must be downloaded first */
#define ACRCLOUD_REUSE 0
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

static audioidparse_t acrmbtrackjp [] = {
  { AUDIOID_PARSE_DATA,   TAG_RECORDING_ID, "id", NULL, NULL, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-mb-track", NULL, NULL, NULL },
};

static audioidparse_t acrmbjp [] = {
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "track", NULL, acrmbtrackjp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-mb", NULL, NULL, NULL },
};

static audioidparse_t acrextmetajp [] = {
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "musicbrainz", NULL, acrmbjp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_TREE, "end-ext-meta", NULL, NULL, NULL },
};

static audioidparse_t acrartistsjp [] = {
  /* as this is an array, the joinphrase will be set for each new */
  /* artist object */
  { AUDIOID_PARSE_SET,    AUDIOID_TYPE_JOINPHRASE, ", ", NULL, NULL, NULL },
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
  { AUDIOID_PARSE_TREE,   AUDIOID_TYPE_TREE, "external_metadata", NULL, acrextmetajp, NULL },
  { AUDIOID_PARSE_END,    AUDIOID_TYPE_ARRAY, "end-music", NULL, NULL, NULL },
};

static audioidparse_t acrmetadatajp [] = {
  { AUDIOID_PARSE_ARRAY,  AUDIOID_TYPE_TOP, "music", NULL, acrmusicjp, NULL },
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

static void acrWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm);
static void dumpData (audioidacr_t *acr);

audioidacr_t *
acrInit (void)
{
  audioidacr_t    *acr;
  const char      *tver;
  const char      *tmp;

  acr = mdmalloc (sizeof (audioidacr_t));
  acr->webclient = webclientAlloc (acr, acrWebResponseCallback);
  acr->webresponse = NULL;
  acr->webresplen = 0;
  mstimeset (&acr->globalreqtimer, 0);
  acr->globalreqcount = 0;

  *acr->key = '\0';
  tmp = bdjoptGetStr (OPT_G_ACRCLOUD_API_KEY);
  if (tmp != NULL) {
    vsencdec (tmp, acr->key, sizeof (acr->key));
  }

  *acr->secret = '\0';
  tmp = bdjoptGetStr (OPT_G_ACRCLOUD_API_SECRET);
  if (tmp != NULL) {
    vsencdec (tmp, acr->secret, sizeof (acr->secret));
  }
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
acrLookup (audioidacr_t *acr, const song_t *song, audioid_resp_t *resp)
{
  char            infn [BDJ4_PATH_MAX];
  char            uri [BDJ4_PATH_MAX];
  char            sig [BDJ4_PATH_MAX];
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
  const char      *acrextr;
  int             targc = 0;
  const char      *fn;
  char            ffn [BDJ4_PATH_MAX];
  char            fpfn [BDJ4_PATH_MAX];
  mstime_t        starttm;
  int             rc;
  const char      *tstr;
  nlist_t         *respdata;

  if (! *acr->key || ! *acr->secret) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: not configured");
    return 0;
  }

  acrextr = sysvarsGetStr (SV_PATH_ACRCLOUD);
  if (acrextr == NULL || ! *acrextr) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: no acr_extr executable");
    return 0;
  }

  fn = songGetStr (song, TAG_URI);
  if (audiosrcGetType (fn) != AUDIOSRC_TYPE_FILE) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: not an audio file");
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

  audiosrcFullPath (fn, ffn, sizeof (ffn), NULL, 0);
  if (! fileopFileExists (ffn)) {
    return 0;
  }
  snprintf (infn, sizeof (infn), "%s%s",
      ffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT));
  /* check for .original filename */
  if (! fileopFileExists (infn)) {
    stpecpy (infn, infn + sizeof (infn), ffn);
  }

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
  mdextalloc (b64sig);

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

#if ACRCLOUD_REUSE
  if (fileopFileExists (ACRCLOUD_TEMP_FN)) {
    FILE    *ifh;
    size_t  tsize;
    char    *tstr;

    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: ** re-using web response");
    /* debugging :  re-use out-acr.json file as input rather */
    /*              than making another query */
    tsize = fileopSize (ACRCLOUD_TEMP_FN);
    ifh = fopen (ACRCLOUD_TEMP_FN, "r");
    acr->webresplen = tsize;
    /* this will leak */
    tstr = malloc (tsize + 1);
    (void) ! fread (tstr, tsize, 1, ifh);
    tstr [tsize] = '\0';
    acr->webresponse = tstr;
    fclose (ifh);
  } else
#endif
  {
    int             webrc;

    mstimestart (&starttm);
    webclientSetTimeout (acr->webclient, 20000);
    webrc = webclientUploadFile (acr->webclient, uri, query, fpfn, "sample");
    logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: web-query: %d %" PRId64 "ms",
        webrc, (int64_t) mstimeend (&starttm));
    if (webrc != WEB_OK) {
      return 0;
    }

    if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
      dumpData (acr);
    }
  }

  gcry_mac_close (gch);
  mdextfree (b64sig);
  free (b64sig);

  mstimestart (&starttm);
  audioidParseJSONAll (acr->webresponse, acr->webresplen,
      acrmainstatusjp, resp, AUDIOID_ID_ACRCLOUD);
  respdata = audioidGetResponseData (resp, resp->respidx);
  tstr = nlistGetStr (respdata, AUDIOID_TYPE_STATUS_CODE);
  acr->respcount = 0;
  rc = -1;
  if (tstr != NULL) {
    rc = atoi (tstr);
  }
  if (rc == 0) {
    acr->respcount = audioidParseJSONAll (acr->webresponse, acr->webresplen,
        acrmainjp, resp, AUDIOID_ID_ACRCLOUD);
  } else if (rc != -1) {
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
acrWebResponseCallback (void *userdata, const char *respstr, size_t len, time_t tm)
{
  audioidacr_t *acr = userdata;

  acr->webresponse = respstr;
  acr->webresplen = len;
  return;
}

static void
dumpData (audioidacr_t *acr)
{
  FILE *ofh;

  if (acr->webresponse != NULL && acr->webresplen > 0) {
    ofh = fopen (ACRCLOUD_TEMP_FN, "w");
    if (ofh != NULL) {
      fwrite (acr->webresponse, 1, acr->webresplen, ofh);
      fprintf (ofh, "\n");
      fclose (ofh);
    }
  }
}
