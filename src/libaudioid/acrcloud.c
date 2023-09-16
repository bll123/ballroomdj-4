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
#include "fileop.h"
#include "localeutil.h"
#include "log.h"
#include "mdebug.h"
#include "song.h"
#include "sysvars.h"
#include "webclient.h"

static void acrWebResponseCallback (void *userdata, const char *resp, size_t len);

typedef struct audioidacr {
  const char    *key;
  const char    *secret;
  webclient_t   *webclient;
  const char    *webresponse;
  size_t        secretlen;
  size_t        webresplen;
} audioidacr_t;

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

void
acrLookup (audioidacr_t *acr, const song_t *song, ilist_t *respdata)
{
  char            uri [MAXPATHLEN];
  char            sig [MAXPATHLEN];
  unsigned char   digest [100];
  size_t          rdlen;
  time_t          tm;
  char            *b64sig;
  gcry_mac_hd_t   gch;
  long            fsize;
  char            tmp [40];
  char            ts [40];
  const char      *query [40];
  int             qc = 0;
  char            *fpfn = NULL;

#if BDJ4_MEM_DEBUG
  mdebugInit ("acrt");
#endif

  tm = time (NULL);

/*
../packages/acrcloud-linux \
  -i $HOME/../music/m/01.Ahe_Ahe_Orquesta_La_Palabra.mp3  \
  -cli -o out.txt
*/

  snprintf (uri, sizeof (uri), "%s", bdjoptGetStr (OPT_G_ACRCLOUD_API_HOST));

  /* no newline at the end */
  snprintf (sig, sizeof (sig), "POST\n/v1/identify\n%s\nfingerprint\n1\n%ld",
      acr->key, tm);
fprintf (stdout, "sig: %s\n", sig);
  gcry_mac_open (&gch, GCRY_MAC_HMAC_SHA1, 0, NULL);
  gcry_mac_setkey (gch, acr->secret, acr->secretlen);
  gcry_mac_write (gch, sig, strlen (sig));
  gcry_mac_read (gch, digest, &rdlen);
fprintf (stdout, "rdlen: %ld\n", rdlen);
  b64sig = g_base64_encode (digest, rdlen);
fprintf (stdout, "digest: %s\n", b64sig);

  fsize = fileopSize (fpfn);
  snprintf (tmp, sizeof (tmp), "%ld", fsize);
fprintf (stdout, "fsize: %s\n", tmp);
  snprintf (ts, sizeof (ts), "%ld", tm);
fprintf (stdout, "ts: %s\n", ts);

  query [qc++] = "sample_bytes";
  query [qc++] = tmp;
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

  webclientUploadFile (acr->webclient, uri, query, fpfn, "sample");
  fprintf (stdout, "resp: %s\n", acr->webresponse);

  gcry_mac_close (gch);
  dataFree (b64sig);

  return;
}

static void
acrWebResponseCallback (void *userdata, const char *resp, size_t len)
{
  audioidacr_t *acr = userdata;

  acr->webresponse = resp;
  acr->webresplen = len;
  return;
}
