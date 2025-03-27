/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h> // for mongoose
#include <dirent.h> // for mongoose
#include <ctype.h>

#include <unistd.h>

#include "mongoose.h"

#include "log.h"
#include "mdebug.h"
#include "websrv.h"

typedef struct websrvint websrvint_t;

typedef struct websrv {
  void                    *userdata;
  websrv_handler_t        handler;
  struct mg_mgr           mgr;
  struct mg_connection    *conn;      // temporary
  struct mg_http_message  *httpmsg;   // temporary
  struct mg_str           cdata;
  struct mg_str           kdata;
  bool                    tlsflag;
} websrv_t;

static void websrvEventHandler (struct mg_connection *c, int ev, void *ev_data);
static void websrvLog (char c, void *userdata);

websrv_t *
websrvInit (uint16_t listenPort, websrv_handler_t eventHandler,
    void *userdata, bool tlsflag)
{
  websrv_t        *websrv;
  char            tbuff [100];

  websrv = mdmalloc (sizeof (websrv_t));

  mg_log_set_fn (websrvLog, NULL);
  if (logCheck (LOG_DBG, LOG_WEBSRV)) {
    mg_log_set (3);
  }

  mg_mgr_init (&websrv->mgr);
  websrv->userdata = userdata;
  websrv->handler = eventHandler;
  websrv->conn = NULL;
  websrv->httpmsg = NULL;
  websrv->tlsflag = tlsflag;
  websrv->cdata.buf = NULL;
  websrv->cdata.len = 0;
  websrv->kdata.buf = NULL;
  websrv->kdata.len = 0;

  if (tlsflag == WEBSRV_TLS_OFF) {
    snprintf (tbuff, sizeof (tbuff), "http://0.0.0.0:%" PRIu16, listenPort);
    mg_http_listen (&websrv->mgr, tbuff, websrvEventHandler, websrv);
  }
  if (tlsflag == WEBSRV_TLS_ON) {
// ### need full path
    websrv->cdata = mg_file_read (&mg_fs_posix, "http/server.crt");
    websrv->kdata = mg_file_read (&mg_fs_posix, "http/server.key");
    snprintf (tbuff, sizeof (tbuff), "https://0.0.0.0:%" PRIu16, listenPort);
    mg_http_listen (&websrv->mgr, tbuff, websrvEventHandler, websrv);
  }
  return websrv;
}

void
websrvFree (websrv_t *websrv)
{
  if (websrv == NULL) {
    return;
  }

  mg_mgr_free (&websrv->mgr);
  if (websrv->cdata.buf != NULL) {
    free (websrv->cdata.buf);
  }
  if (websrv->kdata.buf != NULL) {
    free (websrv->kdata.buf);
  }
  mdfree (websrv);
}

void
websrvProcess (websrv_t *websrv)
{
  if (websrv == NULL) {
    return;
  }

  mg_mgr_poll (&websrv->mgr, 10);
}

void
websrvReply (websrv_t *websrv, int httpcode, const char *headers, const char *msg)
{
  if (websrv->conn == NULL) {
    return;
  }
  mg_http_reply (websrv->conn, httpcode, headers, msg);
}

void
websrvServeFile (websrv_t *websrv, const char *dir, const char *path)
{
  struct mg_http_serve_opts opts;
  struct mg_str             origuri;
  struct mg_str             turi;

  memset (&opts, '\0', sizeof (struct mg_http_serve_opts));
  opts.root_dir = dir;

  turi = mg_str (path);
  origuri = websrv->httpmsg->uri;
  websrv->httpmsg->uri = turi;
  mg_http_serve_dir (websrv->conn, websrv->httpmsg, &opts);
  websrv->httpmsg->uri = origuri;
}

void
websrvGetUserPass (websrv_t *websrv, char *user, size_t usersz, char *pass, size_t passsz)
{
  *user = '\0';
  *pass = '\0';

  if (websrv->httpmsg == NULL) {
    return;
  }

  mg_http_creds (websrv->httpmsg, user, usersz, pass, passsz);
}

/* internal routines */

static void
websrvEventHandler (struct mg_connection *c, int ev, void *ev_data)
{
  struct mg_http_message  *hm;
  char                    query [800];
  char                    uri [400];
  char                    *uriptr;
  websrv_t                *websrv;

  if (c == NULL) {
    return;
  }

  websrv = c->fn_data;
  websrv->conn = c;

  if (websrv->tlsflag == WEBSRV_TLS_ON && ev == MG_EV_ACCEPT) {
    struct mg_tls_opts opts;

    memset (&opts, 0, sizeof (struct mg_tls_opts));

// ### will need to set full path...
    opts.cert = websrv->cdata;
    opts.key = websrv->kdata;
    mg_tls_init (c, &opts);
    return;
  }

  if (ev != MG_EV_HTTP_MSG) {
    return;
  }

  hm = (struct mg_http_message *) ev_data;
  websrv->httpmsg = hm;

  mg_url_decode (hm->uri.buf, hm->uri.len, uri, sizeof (uri), 1);
  uriptr = uri + hm->uri.len - 4;
  if (strcmp (uriptr, ".key") == 0 ||
      strcmp (uriptr, ".crt") == 0 ||
      strcmp (uriptr, ".pem") == 0 ||
      strcmp (uriptr, ".csr") == 0 ||
      strncmp (uri, "../", 3) == 0) {
    mg_http_reply (c, WEB_FORBIDDEN, NULL, WEB_RESP_FORBIDDEN);
    return;
  }

  if (hm->method.len >= 3 && strncmp ("GET", hm->method.buf, 3) == 0) {
    mg_url_decode (hm->query.buf, hm->query.len, query, sizeof (query), 1);
  }
  if (hm->method.len >= 4 && strncmp ("POST", hm->method.buf, 4) == 0) {
    mg_url_decode (hm->body.buf, hm->body.len, query, sizeof (query), 1);
  }

  if (websrv->handler != NULL) {
    websrv->handler (websrv->userdata, query, uri);
  }
}

static void
websrvLog (char c, void *userdata)
{
  static char   wlogbuff [2048];
  static size_t len = 0;

  if (c == '\r') {
    return;
  }
  if (c == '\n' || len >= (sizeof (wlogbuff) - 1)) {
    wlogbuff [len] = '\0';
    logMsg (LOG_DBG, LOG_WEBSRV, "%s", wlogbuff);
    len = 0;
    return;
  }
  if (! isprint (c)) {
    return;
  }

  wlogbuff [len++] = c;
  wlogbuff [len] = '\0';
}
