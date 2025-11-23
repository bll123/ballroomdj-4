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
#include <errno.h>
#include <time.h>

#include <curl/curl.h>
#include <glib.h>
#include <zlib.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "log.h"
#include "mdebug.h"
#include "filedata.h"
#include "fileop.h"
#include "sysvars.h"
#include "tmutil.h"
#include "webclient.h"

enum {
  SUPPORT_BUFF_SZ = (10*1024*1024),
  SMALL_BUFF_SZ = (1*1024*1024),
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
  time_t          respTime;
  char            errstr [CURL_ERROR_SIZE];
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
static void     webclientCleanup (void);

static int initialized = 0;

webclient_t *
webclientAlloc (void *userdata, webclientcb_t callback)
{
  webclient_t   *webclient;

  webclient = mdmalloc (sizeof (webclient_t));
  webclient->userdata = userdata;
  webclient->callback = callback;
  webclient->curl = NULL;
  webclient->dlSize = 0;
  webclient->dlChunks = 0;
  webclient->dlFH = NULL;
  webclient->resp = NULL;
  webclient->respAllocated = 0;
  webclient->respSize = 0;
  webclient->respTime = 0;

  if (initialized == 0) {
    curl_global_init (CURL_GLOBAL_ALL);
    atexit (webclientCleanup);
  }
  ++initialized;
  webclientInit (webclient);
  return webclient;
}

int
webclientHead (webclient_t *webclient, const char *uri)
{
  long        respcode;
  curl_off_t  tm;
  CURLcode    res;

  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_FILETIME, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: head: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  curl_easy_getinfo (webclient->curl, CURLINFO_FILETIME_T, &tm);
  webclient->respTime = tm;
  if (respcode != 200) {
    if (webclient->resp != NULL) {
      snprintf (webclient->resp, webclient->respAllocated, "%s: %ld",
          /* CONTEXT: HTTP response code (error) */
          _("HTTP Code"), respcode);
    }
  }
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize, webclient->respTime);
  }

  return (int) respcode;
}

int
webclientGet (webclient_t *webclient, const char *uri)
{
  long        respcode;
  curl_off_t  tm;
  CURLcode    res;

  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_FILETIME, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: get: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  curl_easy_getinfo (webclient->curl, CURLINFO_FILETIME_T, &tm);
  webclient->respTime = tm;
  if (respcode != 200) {
    if (webclient->resp != NULL) {
      snprintf (webclient->resp, webclient->respAllocated, "%s: %ld",
          /* CONTEXT: HTTP response code (error) */
          _("HTTP Code"), respcode);
    }
  }
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize, webclient->respTime);
  }

  return (int) respcode;
}

int
webclientPost (webclient_t *webclient, const char *uri, const char *query)
{
  long        respcode;
  curl_off_t  tm;
  CURLcode    res;

  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_POST, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_POSTFIELDS, query);
  curl_easy_setopt (webclient->curl, CURLOPT_FILETIME, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: post: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  curl_easy_getinfo (webclient->curl, CURLINFO_FILETIME_T, &tm);
  webclient->respTime = tm;
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize, webclient->respTime);
  }

  return (int) respcode;
}

int
webclientPostCompressed (webclient_t *webclient, const char *uri, const char *query)
{
  struct curl_slist *list = NULL;
  size_t            len;
  long              olen;
  char              *obuff;
  z_stream          *zs;
  long              respcode;
  curl_off_t        tm;
  CURLcode          res;

  len = strlen (query);
  obuff = mdmalloc (SMALL_BUFF_SZ);

  zs = webclientGzipInit (obuff, SMALL_BUFF_SZ);
  webclientGzip (zs, query, len);
  olen = webclientGzipEnd (zs);

  list = curl_slist_append (list, "Content-Encoding: gzip");
  webclientInitResp (webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_POST, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt (webclient->curl, CURLOPT_POSTFIELDSIZE, olen);
  curl_easy_setopt (webclient->curl, CURLOPT_POSTFIELDS, obuff);
  curl_easy_setopt (webclient->curl, CURLOPT_FILETIME, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: post-comp: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  curl_easy_getinfo (webclient->curl, CURLINFO_FILETIME_T, &tm);
  webclient->respTime = tm;
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize, webclient->respTime);
  }
  curl_slist_free_all (list);
  mdfree (obuff);

  return (int) respcode;
}

int
webclientDownload (webclient_t *webclient, const char *uri, const char *outfile)
{
  FILE      *fh;
  time_t    tm;
  long      respcode;
  CURLcode  res;

  webclient->dlSize = 0;
  webclient->dlChunks = 0;
  mstimeset (&webclient->dlStart, 0);
  fh = fileopOpen (outfile, "wb");
  if (fh == NULL) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "download: unable to open %s %d %s",
        outfile, errno, strerror (errno));
    return WEB_BAD_REQUEST;
  }
  webclient->dlFH = fh;
  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientDownloadCallback);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: download: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  mdextfclose (fh);
  fclose (fh);
  webclient->dlFH = NULL;
  tm = mstimeend (&webclient->dlStart);
  logMsg (LOG_DBG, LOG_INFO, "%s : %" PRIu64 " bytes; %" PRIu64 " msec; %" PRIu64 " chunks; %.1f b/s",
      outfile, (uint64_t) webclient->dlSize, (uint64_t) tm,
      (uint64_t) webclient->dlChunks,
      (double) webclient->dlSize / (double) tm);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientCallback);

  return (int) respcode;
}

/* can be used without passing a file, set fn to null */
int
webclientUploadFile (webclient_t *webclient, const char *uri,
    const char *query [], const char *fn, const char *fnname)
{
  curl_off_t  speed_upload;
  curl_off_t  total_time;
  curl_off_t  fsize = 0;
  curl_mime   *mime = NULL;
  curl_mimepart *part = NULL;
  const char  *key;
  const char  *val;
  int         count;
  bool        havefile = false;
  long        respcode;
  CURLcode    res;

  if (fileopFileExists (fn)) {
    havefile = true;
  }

  if (havefile) {
    fsize = fileopSize (fn);
    if (fsize == 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "upload: file is empty %s", fn);
      return WEB_BAD_REQUEST;
    }
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
  if (havefile) {
    curl_mime_name (part, fnname);
    curl_mime_filedata (part, fn);
  }

  curl_easy_setopt (webclient->curl, CURLOPT_URL, uri);
  curl_easy_setopt (webclient->curl, CURLOPT_MIMEPOST, mime);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: upload: %s uri: %s", webclient->errstr, uri);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_RESPONSE_CODE, &respcode);
  if (webclient->callback != NULL) {
    webclient->callback (webclient->userdata, webclient->resp, webclient->respSize, 0);
  }

  curl_mime_free (mime);

  curl_easy_getinfo (webclient->curl, CURLINFO_SPEED_UPLOAD_T, &speed_upload);
  curl_easy_getinfo (webclient->curl, CURLINFO_TOTAL_TIME_T, &total_time);

  if (havefile) {
    logMsg (LOG_DBG, LOG_INFO,
        "%s : %" PRIu64 " : %" PRIu64 " b/sec : %" PRIu64 ".%02" PRIu64 " sec",
        fn,
        (uint64_t) fsize,
        (uint64_t) speed_upload,
        (uint64_t) (total_time / 1000000),
        (uint64_t) (total_time % 1000000));
  }

  return (int) respcode;
}

void
webclientClose (webclient_t *webclient)
{
  if (webclient == NULL) {
    return;
  }

  dataFree (webclient->resp);
  webclient->resp = NULL;
  if (webclient->curl != NULL) {
    mdextfree (webclient->curl);
    curl_easy_cleanup (webclient->curl);
  }
  webclient->curl = NULL;
  --initialized;
  mdfree (webclient);
}

void
webclientSetTimeout (webclient_t *webclient, long timeout)
{
  if (webclient == NULL || webclient->curl == NULL) {
    return;
  }

  curl_easy_setopt (webclient->curl, CURLOPT_TIMEOUT_MS, timeout);
}

void
webclientSpoofUserAgent (webclient_t *webclient)
{
  if (webclient == NULL || webclient->curl == NULL) {
    return;
  }

  curl_easy_setopt (webclient->curl, CURLOPT_USERAGENT,
      "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/117.0");
}

void
webclientIgnoreCertErr (webclient_t *webclient)
{
  if (webclient == NULL || webclient->curl == NULL) {
    return;
  }

  /* the peer can be verified as the ca-file is available */
  curl_easy_setopt (webclient->curl, CURLOPT_CAINFO, sysvarsGetStr (SV_CA_FILE_LOCAL));
  //curl_easy_setopt (webclient->curl, CURLOPT_SSL_VERIFYPEER, 0);
  /* the host cannot be verified */
  curl_easy_setopt (webclient->curl, CURLOPT_SSL_VERIFYHOST, 0L);
}


void
webclientGetLocalIP (char *ip, size_t sz)
{
  webclient_t   *webclient;
  char          *tip;
  char          tbuff [MAXPATHLEN];
  CURLcode      res;

  *ip = '\0';

  webclient = webclientAlloc (NULL, NULL);
  snprintf (tbuff, sizeof (tbuff), "%s/%s",
      bdjoptGetStr (OPT_HOST_VERSION), bdjoptGetStr (OPT_URI_VERSION));
  curl_easy_setopt (webclient->curl, CURLOPT_URL, tbuff);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientNullCallback);
  curl_easy_setopt (webclient->curl, CURLOPT_ERRORBUFFER, webclient->errstr);
  res = curl_easy_perform (webclient->curl);
  if (res != CURLE_OK) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: get-local-ip: %s uri: %s", webclient->errstr, tbuff);
    snprintf (webclient->resp, webclient->respAllocated, "%s", webclient->errstr);
  }

  curl_easy_getinfo (webclient->curl, CURLINFO_LOCAL_IP, &tip);
  stpecpy (ip, ip + sz, tip);

  webclientClose (webclient);
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
  /* if the database becomes so large that 10 megs compressed can't hold it */
  /* then there will be a problem */
  obuff = mdmalloc (SUPPORT_BUFF_SZ);

  zs = webclientGzipInit (obuff, SUPPORT_BUFF_SZ);
  while ((r = fread (buff, 1, SUPPORT_BUFF_SZ, infh)) > 0) {
    webclientGzip (zs, buff, r);
  }
  olen = webclientGzipEnd (zs);
  data = g_base64_encode ((const guchar *) obuff, olen);
  mdextalloc (data);
  fwrite (data, strlen (data), 1, outfh);
  mdfree (data);    // allocated by glib

  mdextfclose (infh);
  fclose (infh);
  mdextfclose (outfh);
  fclose (outfh);
  mdfree (buff);
  mdfree (obuff);
}

void
webclientSetUserPass (webclient_t *webclient, const char *user,
    const char *pass)
{
  if (webclient == NULL || webclient->curl == NULL) {
    return;
  }

  curl_easy_setopt (webclient->curl, CURLOPT_USERNAME, user);
  curl_easy_setopt (webclient->curl, CURLOPT_PASSWORD, pass);
}

/* internal routines */

static void
webclientInit (webclient_t *webclient)
{
  webclient->curl = curl_easy_init ();
  mdextalloc (webclient->curl);
  if (logCheck (LOG_DBG, LOG_WEBCLIENT)) {
    curl_easy_setopt (webclient->curl, CURLOPT_DEBUGFUNCTION, webclientDebugCallback);
    curl_easy_setopt (webclient->curl, CURLOPT_VERBOSE, 1L);
  }
  curl_easy_setopt (webclient->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEDATA, webclient);
  curl_easy_setopt (webclient->curl, CURLOPT_WRITEFUNCTION, webclientCallback);
  curl_easy_setopt (webclient->curl, CURLOPT_BUFFERSIZE, 512L * 1024L);
  curl_easy_setopt (webclient->curl, CURLOPT_ACCEPT_ENCODING, "zstd, gzip, deflate");
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

  if (logCheck (LOG_DBG, LOG_WEBCLIENT)) {
    tdata = mdmalloc (size + 1);
    memcpy (tdata, data, size);
    tdata [size] = '\0';
    p = strtok_r (tdata, "\r\n", &tokstr);
    while (p != NULL && p < tdata + size) {
      logMsg (LOG_DBG, LOG_WEBCLIENT, "curl: %s: %s", tag, p);
      p = strtok_r (NULL, "\r\n", &tokstr);
    }
    mdfree (tdata);
  }
  return 0;
}

static void
webclientSetUserAgent (CURL *curl)
{
  char  tbuff [200];

  snprintf (tbuff, sizeof (tbuff),
      "BDJ4/%s ( %s/ )", sysvarsGetStr (SV_BDJ4_VERSION),
      bdjoptGetStr (OPT_URI_HOMEPAGE));
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

static void
webclientCleanup (void)
{
  if (initialized <= 0) {
    curl_global_cleanup ();
    initialized = 0;
  }
}
