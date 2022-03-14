/*
 *  bdj4dbupdate
 *  updates the database.
 *  there are various modes.
 *    - rebuild
 *      rebuild and replace the database in its entirety
 *    - check-for-new
 *      check for new files and changes and add them.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "filemanip.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "progstate.h"
#include "procutil.h"
#include "slist.h"
#include "sockh.h"

enum {
  DB_UPD_INIT,
  DB_UPD_PREP,
  DB_UPD_SEND,
  DB_UPD_PROCESS,
  DB_UPD_FINISH,
};

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  int               dbgflags;
  int               state;
  slist_t           *fileList;
  slistidx_t        flIterIdx;
  dbidx_t           fileCount;
  dbidx_t           fileCountB;
  dbidx_t           filesProcessed;
} dbupdate_t;

static int      dbupdateProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      dbupdateProcessing (void *udata);
static bool     dbupdateListeningCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateConnectingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateStoppingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateClosingCallback (void *tdbupdate, programstate_t programState);
static void     dbupdateProcessTagData (dbupdate_t *dbupdate, char *args);
static void     dbupdateSigHandler (int sig);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  dbupdate_t    dbupdate;
  uint16_t      listenPort;
  int           flags;


  dbupdate.state = DB_UPD_INIT;
  dbupdate.filesProcessed = 0;
  dbupdate.fileCount = 0;
  dbupdate.fileCountB = 0;

  dbupdate.progstate = progstateInit ("dbupdate");
  progstateSetCallback (dbupdate.progstate, STATE_LISTENING,
      dbupdateListeningCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_CONNECTING,
      dbupdateConnectingCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_WAIT_HANDSHAKE,
      dbupdateHandshakeCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_STOPPING,
      dbupdateStoppingCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_CLOSING,
      dbupdateClosingCallback, &dbupdate);

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    dbupdate.processes [i] = NULL;
  }

#if _define_SIGHUP
  procutilCatchSignal (dbupdateSigHandler, SIGHUP);
#endif
  procutilCatchSignal (dbupdateSigHandler, SIGINT);
  procutilCatchSignal (dbupdateSigHandler, SIGTERM);
#if _define_SIGCHLD
  procutilIgnoreSignal (SIGCHLD);
#endif

  flags = BDJ4_INIT_NONE;
  if ((dbupdate.dbgflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
    flags = BDJ4_INIT_NO_DB_LOAD;
  }
  dbupdate.dbgflags = bdj4startup (argc, argv, "db", ROUTE_DBUPDATE, flags);
  logProcBegin (LOG_PROC, "dbupdate");

  dbupdate.conn = connInit (ROUTE_DBUPDATE);

  listenPort = bdjvarsGetNum (BDJVL_DBUPDATE_PORT);
  sockhMainLoop (listenPort, dbupdateProcessMsg, dbupdateProcessing, &dbupdate);

  while (progstateShutdownProcess (dbupdate.progstate) != STATE_CLOSED) {
    ;
  }
  progstateFree (dbupdate.progstate);

  logProcEnd (LOG_PROC, "dbupdate", "");
  logEnd ();
  return 0;
}

/* internal routines */

static int
dbupdateProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  dbupdate_t      *dbupdate;

  dbupdate = (dbupdate_t *) udata;

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_DBUPDATE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (dbupdate->conn, routefrom);
          connConnectResponse (dbupdate->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (dbupdate->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (dbupdate->progstate);
          logProcEnd (LOG_PROC, "dbupdateProcessMsg", "req-exit");
          return 1;
        }
        case MSG_DB_FILE_TAGS: {
          dbupdateProcessTagData (dbupdate, args);
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
dbupdateProcessing (void *udata)
{
  dbupdate_t  *dbupdate = (dbupdate_t *) udata;

  if (! progstateIsRunning (dbupdate->progstate)) {
    progstateProcess (dbupdate->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  if (dbupdate->state == DB_UPD_INIT) {
    /* for a rebuild, open up a new database, and then just do all */
    /* the normal processing */
    if ((dbupdate->dbgflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
      char  tbuff [MAXPATHLEN];

      dbClose ();
      pathbldMakePath (tbuff, sizeof (tbuff), "",
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_NONE);
      dbOpen (tbuff);
    }

    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    char    *musicdir;

    /* send status message first, as this process will be locked up */
    /* reading the filesystem */
// ###

    musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->fileList = filemanipRecursiveDirList (musicdir);
    dbupdate->fileCount = slistGetCount (dbupdate->fileList);
    slistStartIterator (dbupdate->fileList, &dbupdate->flIterIdx);
    dbupdate->state = DB_UPD_SEND;
  }

  if (dbupdate->state == DB_UPD_SEND) {
    int     count = 0;
    char    *fn;

    while ((fn =
        slistIterateKey (dbupdate->fileList, &dbupdate->flIterIdx)) != NULL) {
      connSendMessage (dbupdate->conn, ROUTE_DBTAG, MSG_DB_FILE_CHK, fn);
      ++dbupdate->fileCountB;
      ++count;
      if (count > 10) {
        break;
      }
    }

    if (fn == NULL) {
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_PROCESS) {
    if (dbupdate->filesProcessed >= dbupdate->fileCount) {
      dbupdate->state = DB_UPD_FINISH;
    }
  }

  if (dbupdate->state == DB_UPD_FINISH) {
    if ((dbupdate->dbgflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
      /* rename the database files */
      dbClose ();

    }
    return 1;
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static bool
dbupdateListeningCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           flags;

  logProcBegin (LOG_PROC, "dbupdateListeningCallback");

  flags = PROCUTIL_DETACH;
  if ((dbupdate->dbgflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }

  if ((dbupdate->dbgflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    dbupdate->processes [ROUTE_DBTAG] = procutilStartProcess (
        ROUTE_PLAYER, "bdj4dbtag", flags);
  }

  logProcEnd (LOG_PROC, "dbupdateListeningCallback", "");
  return true;
}

static bool
dbupdateConnectingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  bool          rc = false;

  logProcBegin (LOG_PROC, "dbupdateConnectingCallback");

  if ((dbupdate->dbgflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (dbupdate->conn, ROUTE_DBTAG)) {
      connConnect (dbupdate->conn, ROUTE_DBTAG);
    }
  }

  if (connIsConnected (dbupdate->conn, ROUTE_DBTAG)) {
    rc = true;
  }

  logProcEnd (LOG_PROC, "dbupdateConnectingCallback", "");
  return rc;
}

static bool
dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  bool          rc = false;

  logProcBegin (LOG_PROC, "dbupdateHandshakeCallback");

  if (connHaveHandshake (dbupdate->conn, ROUTE_DBTAG)) {
    rc = true;
  }
  logProcEnd (LOG_PROC, "dbupdateHandshakeCallback", "");
  return rc;
}

static bool
dbupdateStoppingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateStoppingCallback");

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (dbupdate->processes [i] != NULL) {
      procutilStopProcess (dbupdate->processes [i], dbupdate->conn, i, false);
    }
  }
  mssleep (200);

  logProcEnd (LOG_PROC, "dbupdateStoppingCallback", "");
  return true;
}

static bool
dbupdateClosingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateClosingCallback");

  bdj4shutdown (ROUTE_DBUPDATE);

  /* give the other processes some time to shut down */
  mssleep (200);
  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    if (dbupdate->processes [i] != NULL) {
      procutilStopProcess (dbupdate->processes [i], dbupdate->conn, i, true);
      procutilFree (dbupdate->processes [i]);
    }
  }

  slistFree (dbupdate->fileList);
  connFree (dbupdate->conn);

  logProcEnd (LOG_PROC, "dbupdateClosingCallback", "");
  return true;
}

static void
dbupdateProcessTagData (dbupdate_t *dbupdate, char *args)
{
  slist_t   *tagdata;
  char      *ffn;
  char      *data;
  char      *tokstr;


  ffn = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  data = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);

  tagdata = audiotagParseData (ffn, data);
  // ### do something with it.
  slistFree (tagdata);
  ++dbupdate->filesProcessed;
}

static void
dbupdateSigHandler (int sig)
{
  gKillReceived = 1;
}
