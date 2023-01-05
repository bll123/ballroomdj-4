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

#include <curl/curl.h>
#include <glib.h>
#include <zlib.h>

#include "bdj4.h"
#include "log.h"
#include "mdebug.h"
#include "filedata.h"
#include "fileop.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

enum {
  INIT_NONE,
  INIT_CLIENT,
};

enum {
  SUPPORT_BUFF_SZ = (10*1024*1024),
};

typedef struct webclient {
  void            *userdata;
  webclientcb_t   callback;
  CURL            *curl;
  size_t          dlSize;
  size_t          dlChunks;
  mstime_t        dlStart;
  FILE            *dlFH;
  char            *resp;
  size_t          respAllocated;
  size_t          respSize;
} webclient_t;

static void   webclientInit (webclient_t *webclient);
static void   webclientInitResp (webclient_t *webclient);
static size_t webclientCallback (char *ptr, size_t size, size_t nmemb, void *userdata);
static size_t webclientNullCallback (char *ptr, size_t size, size_t nmemb, void *userdata);
static size_t webclientDownloadCallback (char *ptr, size_t size, size_t nmemb, void *userdata);
static int webclientDebugCallback (CURL *curl, curl_infotype type, char *data, size_t size, void *userptr);
static void webclientSetUserAgent (CURL *curl);
static z_stream * webclientGzipInit (char *out, int outsz);
static void     webclientGzip (z_stream *zs, const char* in, int insz);
static size_t   webclientGzipEnd (z_stream *zs);

static int initialized = INIT_NONE;

webclient_t *
webclientAlloc (void *userdata, webclientcb_t callback)
{
  webclient_t   *webclient;

  webclient = mdmalloc (sizeof (webclient_t));
  assert (webclient != NULL);
  webclient->userdata = userdata;
  webclient->callback = callback;
  webclient->curl = NULL;
  webclient->dlSize = 0;
  webclient->dlChunks = 0;
  webclient->dlFH = NULL;
  webclient->resp = NULL;
  webclient->respAllocated = 0;
  webclient->respSize = 0;

  if (! initialized) {
    curl_global_init (CURL_GLOBAL_ALL);
    initialized = INIT_CLIENT;
  }
  webclientInit (webclient);
  return webclient;
}

void
webclientGet (webclient_t *webclient, const char *uri)
{
  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_perform (webclient->curl);
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize);
  }
}

void
webclientPost (webclient_t *webclient, const char *uri, const char *query)
{
  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_POST, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_POSTFIELDS, query);
  curl_easy_perform (webclient->curl);
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize);
  }
}

void
webclientDownload (webclient_t *webclient, const char *uri, const char *outfile)
{
  FILE    *fh;
  time_t  tm;

  webclient->dlSize = 0;
  webclient->dlChunks = 0;
  mstimeset (&webclient->dlStart, 0);
  fh = fileopOpen (outfile, "wb");
  if (fh == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "download: unable to open %s %d %s\n",
        outfile, errno, strerror (errno));
    return;
  }
  webclient->dlFH = fh;
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientDownloadCallback);
  curl_easy_perform (webclient->curl);
  fclose (fh);
  webclient->dlFH = NULL;
  tm = mstimeend (&webclient->dlStart);
  logMsg (LOG_DBG, LOG_MAIN, "%s : %"PRIu64" bytes; %"PRIu64" msec; %"PRIu64" chunks; %.1f b/s",
      outfile, (uint64_t) webclient->dlSize, (uint64_t) tm,
      (uint64_t) webclient->dlChunks,
      (double) webclient->dlSize / (double) tm);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientCallback);
}

void
webclientUploadFile (webclient_t *webclient, const char *uri,
    const char *query [], const char *fn)
{
  curl_off_t  speed_upload;
  curl_off_t  total_time;
  curl_off_t  fsize;
  curl_mime   *mime = NULL;
  curl_mimepart *part = NULL;
  const char  *key;
  const char  *val;
  int         count;

  if (! fileopFileExists (fn)) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "upload: no file %s", fn);
    return;
  }

  fsize = fileopSize (fn);
  if (fsize == 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "upload: file is empty %s", fn);
    return;
  }

  webclientInitResp (webclient);
  mime = curl_mime_init (webclient->curl);

  count = 0;
  while ((key = query [count]) != NULL) {
    val = query [count + 1];
    part = curl_mime_addpart (mime);
    curl_mime_name (part, key);
    curl_mime_data (part, val, CURL_ZERO_TERMINATED);
    count += 2;
  }

  part = curl_mime_addpart (mime);
  curl_mime_name (part, "upfile");
  curl_mime_filedata (part, fn);

  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_MIMEPOST, mime);
  curl_easy_perform (webclient->curl);
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize);
  }

  curl_mime_free (mime);

  curl_easy_getinfo (webclient->curl, CURLINFO_SPEED_UPLOAD_T, &speed_upload);
  curl_easy_getinfo (webclient->curl, CURLINFO_TOTAL_TIME_T, &total_time);

  logMsg (LOG_DBG, LOG_MAIN,
      "%s : %"PRIu64" : %"PRIu64" b/sec : %"PRIu64".%02"PRIu64" sec",
      fn,
      (uint64_t) fsize,
      (uint64_t) speed_upload,
      (uint64_t) (total_time / 1000000),
      (uint64_t) (total_time % 1000000));
}

void
webclientClose (webclient_t *webclient)
{
  if (webclient != NULL) {
    dataFree (webclient->resp);
    webclient->resp = NULL;
    if (webclient->curl != NULL) {
      curl_easy_cleanup (webclient->curl);
    }
    webclient->curl = NULL;
    if (initialized == INIT_CLIENT) {
      curl_global_cleanup ();
    }
    mdfree (webclient);
    initialized = INIT_NONE;
  }
}

char *
webclientGetLocalIP (void)
{
  webclient_t   *webclient;
  char  *tip;
  char  *ip;
  char  tbuff [MAXPATHLEN];

  webclient = webclientAlloc (NULL, NULL);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      sysvarsGetStr (SV_HOST_WEB), sysvarsGetStr (SV_WEB_VERSION_FILE));
  curl_easy_setopt (webclient->curl, CURLOPT_URL, tbuff);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientNullCallback);
  curl_easy_perform (webclient->curl);
  curl_easy_getinfo (webclient->curl, CURLINFO_LOCAL_IP, &tip);
  ip = mdstrdup (tip);
  curl_easy_cleanup (webclient->curl);
  if (initialized == INIT_NONE) {
    curl_global_cleanup ();
  }
  mdfree (webclient);

  return ip;
}

void
webclientCompressFile (const char *infn, const char *outfn)
{
  FILE      *infh = NULL;
  FILE      *outfh = NULL;
  char      *buff;
  char      *obuff;
  char      *data;
  size_t    r;
  size_t    olen;
  z_stream  *zs;

  infh = fileopOpen (infn, "rb");
  if (infh == NULL) {
    return;
  }
  outfh = fileopOpen (outfn, "wb");
  if (outfh == NULL) {
    return;
  }

  buff = mdmalloc (SUPPORT_BUFF_SZ);
  assert (buff != NULL);
  /* if the database becomes so large that 10 megs compressed can't hold it */
  /* then there will be a problem */
  obuff = mdmalloc (SUPPORT_BUFF_SZ);
  assert (obuff != NULL);

  zs = webclientGzipInit (obuff, SUPPORT_BUFF_SZ);
  while ((r = fread (buff, 1, SUPPORT_BUFF_SZ, infh)) > 0) {
    webclientGzip (zs, buff, r);
  }
  olen = webclientGzipEnd (zs);
  data = g_base64_encode ((const guchar *) obuff, olen);
  fwrite (data, strlen (data), 1, outfh);
  mdfree (data);

  fclose (infh);
  fclose (outfh);
  mdfree (buff);
  mdfree (obuff);
}

/* internal routines */

static void
webclientInit (webclient_t *webclient)
{
  webclient->curl = curl_easy_init ();
  assert (webclient->curl != NULL);
  if (logCheck (LOG_DBG, LOG_WEBCLIENT)) {
    curl_easy_setopt (webclient->curl, CURLOPT_DEBUGFUNCTION, webclientDebugCallback);
    curl_easy_setopt (webclient->curl, CURLOPT_VERBOSE, 1);
  }
  curl_easy_setopt (webclient->curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt (webclient->curl, CURLOPT_TCP_KEEPALIVE, 1);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEDATA, webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientCallback);
  curl_easy_setopt (webclient->curl, CURLOPT_BUFFERSIZE, 512L * 1024L);
  curl_easy_setopt (webclient->curl, CURLOPT_ACCEPT_ENCODING, "");
  if (isWindows ()) {
    curl_easy_setopt (webclient->curl, CURLOPT_CAINFO, sysvarsGetStr (SV_CA_FILE));
  }
  webclientSetUserAgent (webclient->curl);
}

static void
webclientInitResp (webclient_t *webclient)
{
  if (webclient == NULL) {
    return;
  }

  if (webclient->resp != NULL) {
    webclient->respSize = 0;
    *webclient->resp = '\0';
  }
}

static size_t
webclientCallback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  webclient_t     *webclient = userdata;
  size_t          nsz = size * nmemb;

  while (webclient->respSize + nsz >= webclient->respAllocated) {
    webclient->respAllocated += WEB_RESP_SZ;
    webclient->resp = mdrealloc (webclient->resp, webclient->respAllocated);
  }
  memcpy (webclient->resp + webclient->respSize, ptr, nsz);
  webclient->respSize += nsz;
  webclient->resp [webclient->respSize] = '\0';
  return nsz;
}

static size_t
webclientNullCallback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  return size * nmemb;
}

static size_t
webclientDownloadCallback (char *ptr, size_t size, size_t nmemb, void *userdata)
{
  webclient_t *webclient = userdata;
  size_t  w;

  w = fwrite (ptr, size, nmemb, webclient->dlFH);
  webclient->dlSize += w;
  webclient->dlChunks += 1;
  return w;
}

static int
webclientDebugCallback (CURL *curl, curl_infotype type, char *data,
    size_t size, void *userptr)
{
  char    *p;
  char    *tokstr;
  char    *tag;
  char    *tdata;

  if (type == CURLINFO_SSL_DATA_OUT ||
      type == CURLINFO_SSL_DATA_IN) {
    return 0;
  }

  if (type == CURLINFO_DATA_OUT ||
      type == CURLINFO_DATA_IN) {
    return 0;
  }

  tag = "";
  switch (type) {
    case CURLINFO_SSL_DATA_IN:
    case CURLINFO_SSL_DATA_OUT:
    case CURLINFO_END: {
      break;
    }
    case CURLINFO_TEXT: {
      tag = "Info";
      break;
    }
    case CURLINFO_HEADER_IN: {
      tag = "Header-In";
      break;
    }
    case CURLINFO_HEADER_OUT: {
      tag = "Header-Out";
      break;
    }
    case CURLINFO_DATA_IN: {
      tag = "Data-In";
      break;
    }
    case CURLINFO_DATA_OUT: {
      tag = "Data-Out";
      break;
    }
  }

  tdata = mdmalloc (size + 1);
  memcpy (tdata, data, size);
  tdata [size] = '\0';
  p = strtok_r (tdata, "\r\n", &tokstr);
  while (p != NULL && p < tdata + size) {
    logMsg (LOG_DBG, LOG_WEBCLIENT, "curl: %s: %s", tag, p);
    p = strtok_r (NULL, "\r\n", &tokstr);
  }
  mdfree (tdata);
  return 0;
}

static void
webclientSetUserAgent (CURL *curl)
{
  char  tbuff [100];

  snprintf (tbuff, sizeof (tbuff),
      "BDJ4/%s ( %s/ )", sysvarsGetStr (SV_BDJ4_VERSION),
      sysvarsGetStr (SV_HOST_WEB));
  curl_easy_setopt (curl, CURLOPT_USERAGENT, tbuff);
}

static z_stream *
webclientGzipInit (char *out, int outsz)
{
  z_stream *zs;

  zs = mdmalloc (sizeof (z_stream));
  zs->zalloc = Z_NULL;
  zs->zfree = Z_NULL;
  zs->opaque = Z_NULL;
  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;
  zs->avail_out = (uInt) outsz;
  zs->next_out = (Bytef *) out;

  // hard to believe they don't have a macro for gzip encoding, "Add 16" is the best thing zlib can do:
  // "Add 16 to windowBits to write a simple gzip header and trailer around the compressed data instead of a zlib wrapper"
  deflateInit2 (zs, 6, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
  return zs;
}

static void
webclientGzip (z_stream *zs, const char* in, int insz)
{
  zs->avail_in = (uInt) insz;
  zs->next_in = (Bytef *) in;

  deflate (zs, Z_NO_FLUSH);
}

static size_t
webclientGzipEnd (z_stream *zs)
{
  size_t    olen;

  zs->avail_in = (uInt) 0;
  zs->next_in = (Bytef *) NULL;

  deflate (zs, Z_FINISH);
  olen = zs->total_out;
  deflateEnd (zs);
  mdfree (zs);
  return olen;
}

