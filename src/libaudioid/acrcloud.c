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

#include <glib.h>
#include <gcrypt.h>

#include "audioid.h"
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
#include "songutil.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

typedef struct audioidacr {
  const char    *key;
  const char    *secret;
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        secretlen;
  size_t        webresplen;
} audioidacr_t;

static void acrWebResponseCallback (void *userdata, const char *resp, size_t len);
static void dumpData (audioidacr_t *acr);

audioidacr_t *
acrInit (void)
{
  audioidacr_t    *acr;

  acr = mdmalloc (sizeof (audioidacr_t));
  acr->webclient = webclientAlloc (acr, acrWebResponseCallback);
  acr->webresponse = NULL;
  acr->webresplen = 0;
  acr->key = bdjoptGetStr (OPT_G_ACRCLOUD_API_KEY);
  acr->secret = bdjoptGetStr (OPT_G_ACRCLOUD_API_SECRET);
  acr->secretlen = strlen (acr->secret);

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
  unsigned char   digest [100];
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

  if (acr->key == NULL || ! *acr->key ||
      acr->secret == NULL || ! *acr->secret) {
    return 0;
  }

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
  logMsg (LOG_DBG, LOG_IMPORTANT, "acrcloud: %" PRId64 "ms",
      (int64_t) mstimeend (&starttm));

  snprintf (uri, sizeof (uri), "https://%s/v1/identify",
      bdjoptGetStr (OPT_G_ACRCLOUD_API_HOST));

  /* no newline at the end */
  snprintf (sig, sizeof (sig), "POST\n/v1/identify\n%s\nfingerprint\n1\n%ld",
      acr->key, tm);
  gcry_mac_open (&gch, GCRY_MAC_HMAC_SHA1, 0, NULL);
  gcry_mac_setkey (gch, acr->secret, acr->secretlen);
  gcry_mac_write (gch, sig, strlen (sig));
  gcry_mac_read (gch, digest, &rdlen);
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

  if (logCheck (LOG_DBG, LOG_AUDIOID_DUMP)) {
    dumpData (acr);
  }

  gcry_mac_close (gch);
  dataFree (b64sig);

  return 0;
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

