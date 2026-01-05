/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "asconf.h"
#include "audiosrc.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvars.h"
#include "bdjvarsdf.h"
#include "conn.h"
#include "fileop.h"
#include "ilist.h"
#include "lock.h"
#include "log.h"
#include "mdebug.h"
#include "msgparse.h"
#include "musicdb.h"
#include "musicq.h"
#include "ossignal.h"
#include "pathbld.h"
#include "playlist.h"
#include "progstate.h"
#include "sock.h"
#include "sockh.h"
#include "songlist.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"
#include "websrv.h"

#define SRV_URI_TEXT  "uri="
enum {
  SRV_URI_LEN = strlen (SRV_URI_TEXT),
};

typedef struct {
  musicdb_t       *musicdb;
  conn_t          *conn;
  progstate_t     *progstate;
  char            *locknm;
  asconf_t        *asconf;
  const char      *user;
  const char      *pass;
  websrv_t        *websrv;
  slist_t         *plNames;
  char            srvuri [BDJ4_PATH_MAX];
  size_t          srvurilen;
  int             stopwaitcount;
  uint16_t        port;
  bool            bdj4enabled;
} bdjsrv_t;

static bool bdjsrvHandshakeCallback (void *udata, programstate_t programState);
static bool bdjsrvStoppingCallback (void *udata, programstate_t programState);
static bool bdjsrvStopWaitCallback (void *udata, programstate_t programState);
static bool bdjsrvClosingCallback (void *udata, programstate_t programState);
static void bdjsrvEventHandler (void *userdata, const char *query, const char *uri);
static int  bdjsrvProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
static int  bdjsrvProcessing (void *udata);
static void bdjsrvSigHandler (int sig);
static void bdjsrvGetPlaylistNames (bdjsrv_t *bdjsrv);
static const char * bdjsrvStrip (bdjsrv_t *bdjsrv, const char *query);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  bdjsrv_t        bdjsrv;
  uint16_t        listenPort;
  uint32_t        flags;
  ilistidx_t      iteridx;
  ilistidx_t      askey;

#if BDJ4_MEM_DEBUG
  mdebugInit ("bdjsrv");
#endif

  osSetStandardSignals (bdjsrvSigHandler);

  flags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &bdjsrv.musicdb, "srv", ROUTE_SERVER, &flags);

  bdjsrv.asconf = bdjvarsdfGet (BDJVDF_AUDIOSRC_CONF);

  asconfStartIterator (bdjsrv.asconf, &iteridx);
  while ((askey = asconfIterate (bdjsrv.asconf, &iteridx)) >= 0) {
    if (asconfGetNum (bdjsrv.asconf, askey, ASCONF_MODE) == ASCONF_MODE_SERVER) {
      int   type;

      type = asconfGetNum (bdjsrv.asconf, askey, ASCONF_TYPE);
      if (type == AUDIOSRC_TYPE_BDJ4) {
        bdjsrv.bdj4enabled = true;
      }
      break;
    }
  }

  bdjsrv.port = asconfGetNum (bdjsrv.asconf, askey, ASCONF_PORT);
  snprintf (bdjsrv.srvuri, sizeof (bdjsrv.srvuri), "%s:%" PRIu16 "/",
      asconfGetStr (bdjsrv.asconf, askey, ASCONF_URI), bdjsrv.port);
  bdjsrv.srvurilen = strlen (bdjsrv.srvuri);
  bdjsrv.user = asconfGetStr (bdjsrv.asconf, askey, ASCONF_USER);
  bdjsrv.pass = asconfGetStr (bdjsrv.asconf, askey, ASCONF_PASS);

  bdjsrv.progstate = progstateInit ("bdjsrv");
  bdjsrv.plNames = NULL;
  bdjsrv.websrv = NULL;
  bdjsrv.stopwaitcount = 0;

  progstateSetCallback (bdjsrv.progstate, PROGSTATE_WAIT_HANDSHAKE,
      bdjsrvHandshakeCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, PROGSTATE_STOPPING,
      bdjsrvStoppingCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, PROGSTATE_STOP_WAIT,
      bdjsrvStopWaitCallback, &bdjsrv);
  progstateSetCallback (bdjsrv.progstate, PROGSTATE_CLOSING,
      bdjsrvClosingCallback, &bdjsrv);

  bdjsrv.conn = connInit (ROUTE_SERVER);
  bdjsrv.websrv = websrvInit (bdjsrv.port, bdjsrvEventHandler,
      &bdjsrv, WEBSRV_TLS_ON);

  listenPort = bdjvarsGetNum (BDJVL_PORT_SERVER);
  sockhMainLoop (listenPort, bdjsrvProcessMsg, bdjsrvProcessing, &bdjsrv);
  connFree (bdjsrv.conn);
  progstateFree (bdjsrv.progstate);
  logEnd ();

#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
  return 0;
}

/* internal routines */

static bool
bdjsrvStoppingCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;

  connDisconnectAll (bdjsrv->conn);
  return STATE_FINISHED;
}

static bool
bdjsrvStopWaitCallback (void *tbdjsrv, programstate_t programState)
{
  bdjsrv_t *bdjsrv = tbdjsrv;
  bool        rc;

  rc = connWaitClosed (bdjsrv->conn, &bdjsrv->stopwaitcount);
  return rc;
}

static bool
bdjsrvClosingCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;

  bdj4shutdown (ROUTE_SERVER, bdjsrv->musicdb);
  slistFree (bdjsrv->plNames);
  websrvFree (bdjsrv->websrv);

  return STATE_FINISHED;
}

static void
bdjsrvEventHandler (void *userdata, const char *query, const char *uri)
{
  bdjsrv_t *bdjsrv = userdata;
  char          user [40];
  char          pass [40];

  websrvGetUserPass (bdjsrv->websrv, user, sizeof (user), pass, sizeof (pass));

  if (*uri) {
    logMsg (LOG_DBG, LOG_INFO, "srv: uri: %s", uri);
    logMsg (LOG_DBG, LOG_INFO, "srv: query: %s", query);
  }

  if (user [0] == '\0' || pass [0] == '\0') {
    websrvReply (bdjsrv->websrv, WEB_UNAUTHORIZED,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4\r\n",
        WEB_RESP_UNAUTH);
  } else if (strcmp (user, bdjsrv->user) != 0 ||
      strcmp (pass, bdjsrv->pass) != 0) {
    websrvReply (bdjsrv->websrv, WEB_UNAUTHORIZED,
        "Content-type: text/plain; charset=utf-8\r\n"
        "WWW-Authenticate: Basic realm=BDJ4\r\n",
        WEB_RESP_UNAUTH);
  } else if (strcmp (uri, "/echo") == 0) {
    websrvReply (bdjsrv->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        "");
  } else if (strcmp (uri, "/plnames") == 0) {
    const char  *plnm = NULL;
    char        tbuff [200];
    char        *rbuff;
    char        *rp;
    char        *rend;
    slistidx_t  iteridx;

    bdjsrvGetPlaylistNames (bdjsrv);

    rbuff = mdmalloc (BDJMSG_MAX);
    rbuff [0] = '\0';
    rp = rbuff;
    rend = rbuff + BDJMSG_MAX;

    slistStartIterator (bdjsrv->plNames, &iteridx);
    while ((plnm = slistIterateKey (bdjsrv->plNames, &iteridx)) != NULL) {
      /* CONTEXT: the name of the history song list */
      if (strcmp (plnm, _("History")) == 0) {
        continue;
      }
      snprintf (tbuff, sizeof (tbuff), "%s%c", plnm, MSG_ARGS_RS);
      rp = stpecpy (rp, rend, tbuff);
    }

    websrvReply (bdjsrv->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        rbuff);
    dataFree (rbuff);
  } else if (strcmp (uri, "/plget") == 0) {
    bool        ok = false;
    const char  *plnm = NULL;
    songlist_t  *sl;
    char        tbuff [BDJ4_PATH_MAX];
    char        *rbuff;
    char        *rp;
    char        *rend;
    ilistidx_t  iteridx;
    ilistidx_t  idx;

    plnm = bdjsrvStrip (bdjsrv, query);
    ok = playlistExists (plnm);

    if (! ok) {
      websrvReply (bdjsrv->websrv, WEB_NOT_FOUND,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_NOT_FOUND);
      logMsg (LOG_DBG, LOG_INFO, "srv: plget: not found %s", plnm);
      return;
    }

    rbuff = mdmalloc (BDJMSG_MAX);
    rbuff [0] = '\0';
    rp = rbuff;
    rend = rbuff + BDJMSG_MAX;

    sl = songlistLoad (plnm);
    songlistStartIterator (sl, &iteridx);
    while ((idx = songlistIterate (sl, &iteridx)) >= 0) {
      const char    *songuri;

      songuri = songlistGetStr (sl, idx, SONGLIST_URI);
      snprintf (tbuff, sizeof (tbuff), "%s%c", songuri, MSG_ARGS_RS);
      // logMsg (LOG_DBG, LOG_INFO, "srv: plget: add %s", songuri);
      rp = stpecpy (rp, rend, tbuff);
    }

    websrvReply (bdjsrv->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        rbuff);
    songlistFree (sl);
  } else if (strcmp (uri, "/songexists") == 0) {
    bool        ok = false;
    const char  *resp = WEB_RESP_NOT_FOUND;
    int         rc = WEB_NOT_FOUND;
    const char  *songuri = NULL;

    songuri = bdjsrvStrip (bdjsrv, query);
    ok = audiosrcExists (songuri);
    if (ok) {
      rc = WEB_OK;
      resp = WEB_RESP_OK;
    }

    logMsg (LOG_DBG, LOG_INFO, "srv: song-exists: %d %s", rc, songuri);
    websrvReply (bdjsrv->websrv, rc,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        resp);
  } else if (strcmp (uri, "/songget") == 0) {
    bool          ok;
    char          ffn [BDJ4_PATH_MAX];
    const char    *songuri;

    songuri = bdjsrvStrip (bdjsrv, query);
    ok = audiosrcExists (songuri);

    if (! ok) {
      websrvReply (bdjsrv->websrv, WEB_NOT_FOUND,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_NOT_FOUND);
      logMsg (LOG_DBG, LOG_INFO, "srv: song-get: not found %s", songuri);
      return;
    }

    audiosrcFullPath (songuri, ffn, sizeof (ffn), NULL, 0);
    logMsg (LOG_DBG, LOG_IMPORTANT, "song-get: serve: %s", ffn);
    websrvServeFile (bdjsrv->websrv, "", ffn);
  } else if (strcmp (uri, "/songtags") == 0) {
    bool        ok;
    const char  *songuri;
    song_t      *song = NULL;
    slist_t     *songtags;
    slistidx_t  iteridx;
    const char  *tag;
    char        tbuff [BDJ4_PATH_MAX];
    char        *rbuff;
    char        *rp;
    char        *rend;

    songuri = bdjsrvStrip (bdjsrv, query);
    ok = audiosrcExists (songuri);

    if (ok) {
      song = dbGetByName (bdjsrv->musicdb, songuri);
    }

    if (! ok || song == NULL) {
      websrvReply (bdjsrv->websrv, WEB_NOT_FOUND,
          "Content-type: text/plain; charset=utf-8\r\n"
          "Cache-Control: max-age=0\r\n",
          WEB_RESP_NOT_FOUND);
      logMsg (LOG_DBG, LOG_IMPORTANT, "song-tags: not found %s", songuri);
      return;
    }

    rbuff = mdmalloc (BDJMSG_MAX);
    rbuff [0] = '\0';
    rp = rbuff;
    rend = rbuff + BDJMSG_MAX;

    songtags = songTagList (song);
    slistStartIterator (songtags, &iteridx);
    while ((tag = slistIterateKey (songtags, &iteridx)) != NULL) {
      const char  *value;

      if (strcmp (tag, tagdefs [TAG_URI].tag) == 0) {
        /* this will be re-set by the caller */
        continue;
      }
      if (strcmp (tag, tagdefs [TAG_PREFIX_LEN].tag) == 0) {
        continue;
      }
      if (strcmp (tag, tagdefs [TAG_SAMESONG].tag) == 0) {
        /* same-song is specific to the database, do not copy over */
        continue;
      }

      value = slistGetStr (songtags, tag);
      // logMsg (LOG_DBG, LOG_INFO, "song-tags: add %s %s", tag, value);
      if (value != NULL && *value) {
        snprintf (tbuff, sizeof (tbuff), "%s%c%s%c",
            tag, MSG_ARGS_RS, value, MSG_ARGS_RS);
        rp = stpecpy (rp, rend, tbuff);
      }
    }

    websrvReply (bdjsrv->websrv, WEB_OK,
        "Content-type: text/plain; charset=utf-8\r\n"
        "Cache-Control: max-age=0\r\n",
        rbuff);
    slistFree (songtags);
    return;
  } else {
    char          path [BDJ4_PATH_MAX];
    const char    *turi = uri;

    if (*uri == '/') {
      ++uri;
    }
    pathbldMakePath (path, sizeof (path), uri, "", PATHBLD_MP_DREL_HTTP);
    if (! fileopFileExists (path)) {
      return;
    }

    logMsg (LOG_DBG, LOG_IMPORTANT, "serve: %s", turi);
    websrvServeFile (bdjsrv->websrv, sysvarsGetStr (SV_BDJ4_DREL_HTTP), turi);
  }
}

static int
bdjsrvProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  bdjsrv_t     *bdjsrv = udata;

  /* this just reduces the amount of stuff in the log */
  if (msg != MSG_PLAYER_STATUS_DATA) {
    logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
        routefrom, msgRouteDebugText (routefrom),
        route, msgRouteDebugText (route), msg, msgDebugText (msg), args);
  }

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_SERVER: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (bdjsrv->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (bdjsrv->progstate);
          break;
        }
        case MSG_DB_ENTRY_UPDATE: {
          dbidx_t   dbidx;

          msgparseDBEntryUpdate (args, &dbidx);
          dbLoadEntry (bdjsrv->musicdb, dbidx);
          break;
        }
        case MSG_DB_ENTRY_REMOVE: {
          dbMarkEntryRemoved (bdjsrv->musicdb, atol (args));
          break;
        }
        case MSG_DB_ENTRY_UNREMOVE: {
          dbClearEntryRemoved (bdjsrv->musicdb, atol (args));
          break;
        }
        case MSG_DB_RELOAD: {
          bdjsrv->musicdb = bdj4ReloadDatabase (bdjsrv->musicdb);
          break;
        }
        default: {
          break;
        }
      }
      break;
    }
    default: {
      break;
    }
  }

  return 0;
}

static int
bdjsrvProcessing (void *udata)
{
  bdjsrv_t *bdjsrv = udata;
  websrv_t      *websrv = bdjsrv->websrv;
  int           stop = SOCKH_CONTINUE;


  if (! progstateIsRunning (bdjsrv->progstate)) {
    progstateProcess (bdjsrv->progstate);
    if (progstateCurrState (bdjsrv->progstate) == PROGSTATE_CLOSED) {
      stop = SOCKH_STOP;
    }
    if (gKillReceived) {
      progstateShutdownProcess (bdjsrv->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (bdjsrv->conn);

  websrvProcess (websrv);

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    progstateShutdownProcess (bdjsrv->progstate);
  }
  return stop;
}

static bool
bdjsrvHandshakeCallback (void *udata, programstate_t programState)
{
  bdjsrv_t   *bdjsrv = udata;
  bool            rc = STATE_NOT_FINISH;

  connProcessUnconnected (bdjsrv->conn);
  rc = STATE_FINISHED;
  return rc;
}

static void
bdjsrvSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
bdjsrvGetPlaylistNames (bdjsrv_t *bdjsrv)
{
  slistFree (bdjsrv->plNames);
  bdjsrv->plNames = playlistGetPlaylistNames (PL_LIST_SONGLIST, NULL);
}

static const char *
bdjsrvStrip (bdjsrv_t *bdjsrv, const char *query)
{
  const char  *songuri;

  songuri = query;
  if (strncmp (songuri, SRV_URI_TEXT, SRV_URI_LEN) == 0) {
    songuri += SRV_URI_LEN;
  }
  return songuri;
}
