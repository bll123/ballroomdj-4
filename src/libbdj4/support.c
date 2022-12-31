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
#include <assert.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "fileop.h"
#include "pathbld.h"
#include "pathutil.h"
#include "support.h"
#include "sysvars.h"
#include "webclient.h"

typedef struct support {
  webclient_t   *webclient;
  char          *webresponse;
} support_t;

static void supportWebResponseCallback (void *userdata, char *resp, size_t len);

support_t *
supportAlloc (void)
{
  support_t   *support;

  support = malloc (sizeof (support_t));
  support->webresponse = NULL;
  support->webclient = webclientAlloc (support, supportWebResponseCallback);
  return support;
}

void
supportFree (support_t *support)
{
  if (support != NULL) {
    webclientClose (support->webclient);
    support->webresponse = NULL;
  }
}

void
supportGetLatestVersion (support_t *support, char *buff, size_t sz)
{
  char  uri [200];

  *buff = '\0';
  if (support == NULL) {
    return;
  }

  snprintf (uri, sizeof (uri), "%s/%s",
      sysvarsGetStr (SV_HOST_WEB), sysvarsGetStr (SV_WEB_VERSION_FILE));
  webclientGet (support->webclient, uri);
  if (support->webresponse != NULL) {
    strlcpy (buff, support->webresponse, sz);
    stringTrim (buff);
  }
}

void
supportSendFile (support_t *support, const char *ident,
    const char *origfn, int compflag)
{
  char        fn [MAXPATHLEN];
  char        uri [1024];
  const char  *query [10];
  int         qc = 0;
  pathinfo_t  *pi = NULL;

  if (support == NULL) {
    return;
  }

  if (! fileopFileExists (origfn)) {
    return;
  }
  strlcpy (fn, origfn, sizeof (fn));
  if (compflag == SUPPORT_COMPRESSED) {
    pi = pathInfo (origfn);
    pathbldMakePath (fn, sizeof (fn),
        pi->filename, ".gz.b64", PATHBLD_MP_DREL_TMP);
    webclientCompressFile (origfn, fn);
  }

  if (! fileopFileExists (fn)) {
    return;
  }

  if (*origfn == '/') {
    /* to handle mac os diagnostics files */
    pathInfoFree (pi);
    pi = pathInfo (origfn);
    origfn = pi->filename;
  }

  snprintf (uri, sizeof (uri), "%s%s",
      sysvarsGetStr (SV_HOST_SUPPORTMSG), sysvarsGetStr (SV_URI_SUPPORTMSG));
  query [qc++] = "key";
  query [qc++] = "9034545";
  query [qc++] = "ident";
  query [qc++] = ident;
  query [qc++] = "origfn";
  query [qc++] = origfn;
  query [qc++] = NULL;
  webclientUploadFile (support->webclient, uri, query, fn);
  if (compflag == SUPPORT_COMPRESSED) {
    fileopDelete (fn);
  }
  pathInfoFree (pi);
}

/* internal routines */

static void
supportWebResponseCallback (void *userdata, char *resp, size_t len)
{
  support_t *support = userdata;

  support->webresponse = resp;
  return;
}
