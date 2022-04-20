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
#include <regex.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "fileop.h"
#include "filemanip.h"
#include "log.h"
#include "musicdb.h"
#include "pathbld.h"
#include "progstate.h"
#include "procutil.h"
#include "rafile.h"
#include "slist.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"

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
  char              *musicdir;
  size_t            musicdirlen;
  slist_t           *fileList;
  slistidx_t        flIterIdx;
  regex_t           fnregex;
  dbidx_t           fileCount;
  dbidx_t           fileCountSent;
  dbidx_t           filesProcessed;
  dbidx_t           countAlready;
  dbidx_t           countBad;
  dbidx_t           countNew;
  dbidx_t           countNullData;
  dbidx_t           countNoTags;
  dbidx_t           countSaved;
  mstime_t          starttm;
  bool              rebuild : 1;
  bool              checknew : 1;
} dbupdate_t;

#define FNAMES_SENT_PER_ITER  30

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
  int           rc;
  char          *p;


  dbupdate.state = DB_UPD_INIT;
  dbupdate.filesProcessed = 0;
  dbupdate.fileCount = 0;
  dbupdate.fileCountSent = 0;
  dbupdate.countAlready = 0;
  dbupdate.countBad = 0;
  dbupdate.countNew = 0;
  dbupdate.countNullData = 0;
  dbupdate.countNoTags = 0;
  dbupdate.countSaved = 0;
  dbupdate.rebuild = false;
  dbupdate.checknew = false;

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

  flags = BDJ4_INIT_NO_DB_LOAD;
  dbupdate.dbgflags = bdj4startup (argc, argv, "db", ROUTE_DBUPDATE, flags);
  logProcBegin (LOG_PROC, "dbupdate");

  p = "[\"\\:]";
  if (isWindows ()) {
    p = "[\"]";
  }
  rc = regcomp (&dbupdate.fnregex, p, REG_NOSUB);
  if (rc != 0) {
    char    ebuff [200];

    regerror (rc, &dbupdate.fnregex, ebuff, sizeof (ebuff));
    logMsg (LOG_DBG, LOG_IMPORTANT, "regcomp failed: %d %s", rc, ebuff);
  }

  if ((dbupdate.dbgflags & BDJ4_DB_CHECK_NEW) == BDJ4_DB_CHECK_NEW) {
    dbupdate.checknew = true;
  }
  if ((dbupdate.dbgflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
    dbupdate.rebuild = true;
  }

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
    if (dbupdate->rebuild) {
      char  tbuff [MAXPATHLEN];

      dbClose ();
      pathbldMakePath (tbuff, sizeof (tbuff),
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_NONE);
      fileopDelete (tbuff);
      dbOpen (tbuff);
      dbStartBatch ();
    }

    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    mstimestart (&dbupdate->starttm);

// ###
    /* send status message first, as this process will be locked up */
    /* reading the filesystem */

    dbupdate->musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->musicdirlen = strlen (dbupdate->musicdir);
    dbupdate->fileList = filemanipRecursiveDirList (dbupdate->musicdir, FILEMANIP_FILES);
    dbupdate->fileCount = slistGetCount (dbupdate->fileList);
    mstimeend (&dbupdate->starttm);
    logMsg (LOG_DBG, LOG_IMPORTANT, "read directory %s: %ld ms",
        dbupdate->musicdir, mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "  %ld files found", dbupdate->fileCount);
    slistStartIterator (dbupdate->fileList, &dbupdate->flIterIdx);
    dbupdate->state = DB_UPD_SEND;
  }

  if (dbupdate->state == DB_UPD_SEND) {
    int     count = 0;
    char    *fn;
    int     rc;

    while ((fn =
        slistIterateKey (dbupdate->fileList, &dbupdate->flIterIdx)) != NULL) {
      if (dbupdate->checknew) {
        // check the filename against the db
        // if found, skip.
      }

      rc = regexec (&dbupdate->fnregex, fn, 0, NULL, 0);
      if (rc == 0) {
        ++dbupdate->countBad;
        continue;
      } else if (rc != REG_NOMATCH) {
        char    ebuff [200];

        regerror (rc, &dbupdate->fnregex, ebuff, sizeof (ebuff));
        logMsg (LOG_DBG, LOG_IMPORTANT, "regexec failed: %d %s", rc, ebuff);
      }

      connSendMessage (dbupdate->conn, ROUTE_DBTAG, MSG_DB_FILE_CHK, fn);
      ++dbupdate->fileCountSent;
      ++dbupdate->countNew;
      ++count;
      if (count > FNAMES_SENT_PER_ITER) {
        break;
      }
    }

    if (fn == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "all filenames sent (%d): %ld ms",
          dbupdate->fileCountSent, mstimeend (&dbupdate->starttm));
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_SEND ||
      dbupdate->state == DB_UPD_PROCESS) {
    if (dbupdate->filesProcessed >= dbupdate->fileCountSent) {
      dbupdate->state = DB_UPD_FINISH;
    }
  }

  if (dbupdate->state == DB_UPD_FINISH) {
    if (dbupdate->rebuild) {
      dbEndBatch ();
      dbClose ();
      /* rename the database files */
    }
    logMsg (LOG_DBG, LOG_IMPORTANT, "finish: %ld ms",
        mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "    found: %lu", dbupdate->fileCount);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     sent: %lu", dbupdate->fileCountSent);
    logMsg (LOG_DBG, LOG_IMPORTANT, "processed: %lu", dbupdate->filesProcessed);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  already: %lu", dbupdate->countAlready);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      bad: %lu", dbupdate->countBad);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      new: %lu", dbupdate->countNew);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     null: %lu", dbupdate->countNullData);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  no tags: %lu", dbupdate->countNoTags);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    saved: %lu", dbupdate->countSaved);
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

  regfree (&dbupdate->fnregex);
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
  char      *relfname;
  char      *data;
  char      *tokstr;


  ffn = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  data = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);
  if (data == NULL) {
// ### fix : need to get what is possible from the pathname
    logMsg (LOG_DBG, LOG_DBUPDATE, "  - null data");
    ++dbupdate->countNullData;
    ++dbupdate->filesProcessed;
    return;
  }

  tagdata = audiotagParseData (ffn, data);
  if (slistGetCount (tagdata) == 0) {
// ### fix : need to get what is possible from the pathname
    logMsg (LOG_DBG, LOG_DBUPDATE, "  - no tags");
    ++dbupdate->countNoTags;
  }

  if (logCheck (LOG_DBG, LOG_DBUPDATE)) {
    slistidx_t  iteridx;
    char        *tag, *data;

    slistStartIterator (tagdata, &iteridx);
    while ((tag = slistIterateKey (tagdata, &iteridx)) != NULL) {
      data = slistGetStr (tagdata, tag);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  %s : %s", tag, data);
    }
  }

  /* the dbWrite() procedure will set the FILE tag */
  relfname = slistGetStr (dbupdate->fileList, ffn);
  dbWrite (relfname, tagdata);
  ++dbupdate->countSaved;

  slistFree (tagdata);
  ++dbupdate->filesProcessed;
}

static void
dbupdateSigHandler (int sig)
{
  gKillReceived = 1;
}
