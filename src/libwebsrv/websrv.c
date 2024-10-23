/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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

#include "mongoose.h"

#include "log.h"
#include "mdebug.h"
#include "websrv.h"

typedef struct websrv {
  void                    *userdata;
  websrv_handler_t        handler;
  struct mg_mgr           mgr;
  struct mg_connection    *conn;      // temporary
  struct mg_http_message  *httpmsg;   // temporary
} websrv_t;

static void websrvEventHandler (struct mg_connection *c, int ev, void *ev_data);
static void websrvLog (char c, void *userdata);

websrv_t *
websrvInit (uint16_t listenPort, websrv_handler_t eventHandler,
    void *userdata)
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

  snprintf (tbuff, sizeof (tbuff), "http://0.0.0.0:%" PRIu16, listenPort);
  mg_http_listen (&websrv->mgr, tbuff, websrvEventHandler, websrv);
  return websrv;
}

void
websrvFree (websrv_t *websrv)
{
  if (websrv == NULL) {
    return;
  }

  mg_mgr_free (&websrv->mgr);
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
  char                    *qstrptr;
  char                    *tokstr;
  const char              *querydata;
  char                    uri [400];
  char                    *uriptr;
  websrv_t                *websrv;

  if (c == NULL) {
    return;
  }
  if (ev != MG_EV_HTTP_MSG) {
    return;
  }

  websrv = c->fn_data;
  websrv->conn = c;

  hm = (struct mg_http_message *) ev_data;
  websrv->httpmsg = hm;

  mg_url_decode (hm->uri.buf, hm->uri.len, uri, sizeof (uri), 1);
  uriptr = uri + strlen (uri) - 4;
  if (strcmp (uriptr, ".key") == 0 ||
      strcmp (uriptr, ".crt") == 0 ||
      strcmp (uriptr, ".pem") == 0 ||
      strcmp (uriptr, ".csr") == 0 ||
      strncmp (uri, "../", 3) == 0) {
    mg_http_reply (c, 403, NULL, "Forbidden");
    return;
  }

  mg_url_decode (hm->query.buf, hm->query.len, query, sizeof (query), 1);
  querydata = query;
  if (*query) {
    size_t      len;

    qstrptr = strtok_r (query, " ", &tokstr);
    len = strlen (query);
    if (hm->query.len > len) {
      querydata = query + len + 1;
    }
  }

  if (websrv->handler != NULL) {
    websrv->handler (websrv->userdata, query, querydata, uri);
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

  wlogbuff [len++] = c;
  wlogbuff [len] = '\0';
}
