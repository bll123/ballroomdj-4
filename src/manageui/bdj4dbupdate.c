/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
/*
 *  bdj4dbupdate
 *  updates the database.
 *  there are various modes.
 *    - rebuild
 *      rebuild and replace the database in its entirety.
 *    - check for new
 *      check for new files and changes and add them.
 *    - update from tags
 *      update db from tags in audio files.
 *      this is the same as checknew, except that all audio files tags
 *      are loaded and updated in the database.
 *      so the processing is similar to a rebuild, but using the
 *      existing database and updating the records in the database.
 *    - write tags
 *      write db tags to the audio files
 *    - reorganize
 *      use the organization settings to reorganize the files.
 *    - update from itunes
 *      update the database data from the data found in itunes.
 *
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
#include <signal.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdj4intl.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjregex.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "dirlist.h"
#include "dirop.h"
#include "fileop.h"
#include "filemanip.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "orgutil.h"
#include "ossignal.h"
#include "pathbld.h"
#include "pathutil.h"
#include "progstate.h"
#include "procutil.h"
#include "rafile.h"
#include "slist.h"
#include "sockh.h"
#include "song.h"
#include "sysvars.h"
#include "tagdef.h"
#include "tmutil.h"

enum {
  DB_UPD_INIT,
  DB_UPD_PREP,
  DB_UPD_SEND,
  DB_UPD_PROCESS,
  DB_UPD_FINISH,
};

enum {
  C_FILE_COUNT,
  C_FILE_PROC,
  C_FILE_SKIPPED,
  C_FILE_SENT,
  C_FILE_RCV,
  C_IN_DB,
  C_NEW,
  C_UPDATED,
  C_WRITE_TAGS,
  C_BAD,
  C_NON_AUDIO,
  C_BDJ_SKIP,
  C_BDJ_OLD_DIR,
  C_NULL_DATA,
  C_NO_TAGS,
  C_MAX,
};

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  int               startflags;
  int               state;
  musicdb_t         *musicdb;
  musicdb_t         *newmusicdb;
  char              *musicdir;
  size_t            musicdirlen;
  char              *dbtopdir;
  size_t            dbtopdirlen;
  mstime_t          outputTimer;
  org_t             *org;
  slist_t           *fileList;
  slistidx_t        filelistIterIdx;
  bdjregex_t        *badfnregex;
  dbidx_t           counts [C_MAX];
  size_t            maxWriteLen;
  mstime_t          starttm;
  int               stopwaitcount;
  char              *olddirlist;
  bool              rebuild : 1;
  bool              checknew : 1;
  bool              progress : 1;
  bool              cli : 1;
  bool              verbose : 1;
  bool              updfromtags : 1;
  bool              writetags : 1;
  bool              reorganize : 1;
  bool              newdatabase : 1;
  bool              dancefromgenre : 1;
  bool              usingmusicdir : 1;
  bool              haveolddirlist : 1;
  bool              stoprequest : 1;
} dbupdate_t;

enum {
  FNAMES_SENT_PER_ITER = 30,
};

static int      dbupdateProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      dbupdateProcessing (void *udata);
static bool     dbupdateListeningCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateConnectingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateStoppingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateStopWaitCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateClosingCallback (void *tdbupdate, programstate_t programState);
static void     dbupdateProcessTagData (dbupdate_t *dbupdate, char *args);
static void     dbupdateWriteTags (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata);
static void     dbupdateSigHandler (int sig);
static void     dbupdateOutputProgress (dbupdate_t *dbupdate);
static const char *dbupdateGetRelativePath (dbupdate_t *dbupdate, const char *fn);
static bool     checkOldDirList (dbupdate_t *dbupdate, const char *fn);
static void     dbupdateIncCount (dbupdate_t *dbupdate, int tag);

static int  gKillReceived = 0;

int
main (int argc, char *argv[])
{
  dbupdate_t    dbupdate;
  uint16_t      listenPort;
  int           flags;
  char          *p;

  dbupdate.state = DB_UPD_INIT;
  dbupdate.musicdb = NULL;
  dbupdate.newmusicdb = NULL;
  for (int i = 0; i < C_MAX; ++i) {
    dbupdate.counts [i] = 0;
  }
  dbupdate.maxWriteLen = 0;
  dbupdate.stopwaitcount = 0;
  dbupdate.rebuild = false;
  dbupdate.checknew = false;
  dbupdate.progress = false;
  dbupdate.cli = false;
  dbupdate.verbose = false;
  dbupdate.updfromtags = false;
  dbupdate.writetags = false;
  dbupdate.reorganize = false;
  dbupdate.newdatabase = false;
  dbupdate.dancefromgenre = false;
  dbupdate.usingmusicdir = true;
  dbupdate.olddirlist = NULL;
  dbupdate.haveolddirlist = false;
  dbupdate.stoprequest = false;
  mstimeset (&dbupdate.outputTimer, 0);
  dbupdate.org = NULL;

  dbupdate.dancefromgenre = bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE);

  dbupdate.progstate = progstateInit ("dbupdate");
  progstateSetCallback (dbupdate.progstate, STATE_LISTENING,
      dbupdateListeningCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_CONNECTING,
      dbupdateConnectingCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_WAIT_HANDSHAKE,
      dbupdateHandshakeCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_STOPPING,
      dbupdateStoppingCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_STOP_WAIT,
      dbupdateStopWaitCallback, &dbupdate);
  progstateSetCallback (dbupdate.progstate, STATE_CLOSING,
      dbupdateClosingCallback, &dbupdate);

  procutilInitProcesses (dbupdate.processes);

  osSetStandardSignals (dbupdateSigHandler);

  flags = BDJ4_INIT_NONE;
  dbupdate.startflags = bdj4startup (argc, argv, &dbupdate.musicdb,
      "db", ROUTE_DBUPDATE, flags);
  logProcBegin (LOG_PROC, "dbupdate");

  dbupdate.olddirlist = bdjoptGetStr (OPT_M_DIR_OLD_SKIP);
  if (dbupdate.olddirlist != NULL && *dbupdate.olddirlist) {
    dbupdate.haveolddirlist = true;
  }

  dbupdate.org = orgAlloc (bdjoptGetStr (OPT_G_ORGPATH));

  /* any file with a double quote or backslash is rejected */
  /* on windows, only the double quote is rejected */
  p = "[\"\\\\]";
  if (isWindows ()) {
    p = "[\"]";
  }
  dbupdate.badfnregex = regexInit (p);

  if ((dbupdate.startflags & BDJ4_DB_CHECK_NEW) == BDJ4_DB_CHECK_NEW) {
    dbupdate.checknew = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
    dbupdate.rebuild = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_UPD_FROM_TAGS) == BDJ4_DB_UPD_FROM_TAGS) {
    dbupdate.updfromtags = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_WRITE_TAGS) == BDJ4_DB_WRITE_TAGS) {
    dbupdate.writetags = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_REORG) == BDJ4_DB_REORG) {
    dbupdate.reorganize = true;
  }
  if ((dbupdate.startflags & BDJ4_PROGRESS) == BDJ4_PROGRESS) {
    dbupdate.progress = true;
  }
  if ((dbupdate.startflags & BDJ4_CLI) == BDJ4_CLI) {
    dbupdate.cli = true;
#if _define_SIGCHLD
    osDefaultSignal (SIGCHLD);
#endif
  }
  if ((dbupdate.startflags & BDJ4_VERBOSE) == BDJ4_VERBOSE) {
    dbupdate.verbose = true;
  }

  dbupdate.conn = connInit (ROUTE_DBUPDATE);

  listenPort = bdjvarsGetNum (BDJVL_DBUPDATE_PORT);
  sockhMainLoop (listenPort, dbupdateProcessMsg, dbupdateProcessing, &dbupdate);
  connFree (dbupdate.conn);
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

  logMsg (LOG_DBG, LOG_MSGS, "got: from:%d/%s route:%d/%s msg:%d/%s args:%s",
      routefrom, msgRouteDebugText (routefrom),
      route, msgRouteDebugText (route), msg, msgDebugText (msg), args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_DBUPDATE: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (dbupdate->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          procutilCloseProcess (dbupdate->processes [routefrom],
              dbupdate->conn, routefrom);
          procutilFreeRoute (dbupdate->processes, routefrom);
          connDisconnect (dbupdate->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          dbupdate->state = DB_UPD_FINISH;
          dbupdate->stoprequest = true;
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (dbupdate->progstate);
          break;
        }
        case MSG_DB_STOP_REQ: {
          logMsg (LOG_DBG, LOG_MAIN, "stop request received");
          dbupdate->state = DB_UPD_FINISH;
          dbupdate->stoprequest = true;
          break;
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
  int         stop = false;
  char        *msg = NULL;


  if (! progstateIsRunning (dbupdate->progstate)) {
    progstateProcess (dbupdate->progstate);
    if (progstateCurrState (dbupdate->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      progstateShutdownProcess (dbupdate->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
      gKillReceived = 0;
    }
    return stop;
  }

  connProcessUnconnected (dbupdate->conn);

  if (dbupdate->state == DB_UPD_INIT) {
    if (dbupdate->rebuild) {
      dbupdate->newdatabase = true;
    }

    dbBackup ();

    if (dbupdate->newdatabase) {
      char  tbuff [MAXPATHLEN];

      pathbldMakePath (tbuff, sizeof (tbuff),
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
      fileopDelete (tbuff);
      dbupdate->newmusicdb = dbOpen (tbuff);
      assert (dbupdate->newmusicdb != NULL);
      dbStartBatch (dbupdate->newmusicdb);
    } else {
      dbStartBatch (dbupdate->musicdb);
    }
    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    char  tbuff [100];
    char  *tstr;

    mstimestart (&dbupdate->starttm);

    dbupdateOutputProgress (dbupdate);

    dbupdate->usingmusicdir = true;
    dbupdate->musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->musicdirlen = strlen (dbupdate->musicdir);
    dbupdate->dbtopdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    tstr = bdjvarsGetStr (BDJV_DB_TOP_DIR);
    if (strcmp (tstr, dbupdate->musicdir) != 0) {
      dbupdate->usingmusicdir = false;
      dbupdate->dbtopdir = tstr;
    }
    dbupdate->dbtopdirlen = strlen (dbupdate->dbtopdir);

    logMsg (LOG_DBG, LOG_BASIC, "dbtopdir %s", dbupdate->dbtopdir);
    dbupdate->fileList = dirlistRecursiveDirList (dbupdate->dbtopdir, FILEMANIP_FILES);

    dbupdate->counts [C_FILE_COUNT] = slistGetCount (dbupdate->fileList);
    mstimeend (&dbupdate->starttm);
    logMsg (LOG_DBG, LOG_IMPORTANT, "read directory %s: %"PRId64" ms",
        dbupdate->dbtopdir, (int64_t) mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "  %u files found", dbupdate->counts [C_FILE_COUNT]);

    /* message to manageui */
    /* CONTEXT: database update: status message */
    snprintf (tbuff, sizeof (tbuff), _("%d files found"), dbupdate->counts [C_FILE_COUNT]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    slistStartIterator (dbupdate->fileList, &dbupdate->filelistIterIdx);
    dbupdate->state = DB_UPD_SEND;
  }

  if (dbupdate->state == DB_UPD_SEND) {
    int         count = 0;
    char        *fn;
    pathinfo_t  *pi;

    while ((fn =
        slistIterateKey (dbupdate->fileList, &dbupdate->filelistIterIdx)) != NULL) {

      pi = pathInfo (fn);
      /* fast skip of some known file extensions that might show up */
      if (pathInfoExtCheck (pi, ".jpg") ||
          pathInfoExtCheck (pi, ".png") ||
          pathInfoExtCheck (pi, ".bak") ||
          pathInfoExtCheck (pi, ".txt") ||
          pathInfoExtCheck (pi, ".svg")) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_NON_AUDIO);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-not-audio");
        pathInfoFree (pi);
        continue;
      }
      if (pathInfoExtCheck (pi, BDJ4_ORIGINAL_EXT)) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_BDJ_SKIP);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-orig/del");
        pathInfoFree (pi);
        continue;
      }
      pathInfoFree (pi);

      if (dbupdate->haveolddirlist &&
          checkOldDirList (dbupdate, fn)) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_BDJ_OLD_DIR);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-old-dir");
        continue;
      }

      /* check to see if the audio file is already in the database */
      /* this is done for all modes except for rebuild */
      /* 'checknew' skips any processing for an audio file */
      /* that is already present */
      if (! dbupdate->rebuild && fn != NULL) {
        const char  *p;

        p = dbupdateGetRelativePath (dbupdate, fn);
        if (dbGetByName (dbupdate->musicdb, p) != NULL) {
          dbupdateIncCount (dbupdate, C_IN_DB);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  in-database (%u) ", dbupdate->counts [C_IN_DB]);

          /* if doing a checknew, no need for further processing */
          /* the file exists, don't change the file or the database */
          if (dbupdate->checknew) {
            dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
            dbupdateOutputProgress (dbupdate);
            ++count;
            if (count > FNAMES_SENT_PER_ITER) {
              break;
            }
            continue;
          }
        }
      }

      if (regexMatch (dbupdate->badfnregex, fn)) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_BAD);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  bad (%u) ", dbupdate->counts [C_BAD]);
        continue;
      }

      connSendMessage (dbupdate->conn, ROUTE_DBTAG, MSG_DB_FILE_CHK, fn);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  send %s", fn);
      dbupdateIncCount (dbupdate, C_FILE_SENT);
      ++count;
      if (count > FNAMES_SENT_PER_ITER) {
        break;
      }
    }

    if (fn == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- skipped (%u)", dbupdate->counts [C_FILE_SKIPPED]);
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- all filenames sent (%u): %"PRId64" ms",
          dbupdate->counts [C_FILE_SENT], (int64_t) mstimeend (&dbupdate->starttm));
      connSendMessage (dbupdate->conn, ROUTE_DBTAG, MSG_DB_ALL_FILES_SENT, NULL);
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_SEND ||
      dbupdate->state == DB_UPD_PROCESS) {
    dbupdateOutputProgress (dbupdate);
    logMsg (LOG_DBG, LOG_DBUPDATE, "progress: %u+%u(%u) >= %u",
        dbupdate->counts [C_FILE_PROC],
        dbupdate->counts [C_FILE_SKIPPED],
        dbupdate->counts [C_FILE_PROC] + dbupdate->counts [C_FILE_SKIPPED],
        dbupdate->counts [C_FILE_COUNT]);
    if (dbupdate->counts [C_FILE_PROC] + dbupdate->counts [C_FILE_SKIPPED] >=
        dbupdate->counts [C_FILE_COUNT]) {
      logMsg (LOG_DBG, LOG_DBUPDATE, "  done");
      dbupdate->state = DB_UPD_FINISH;
      if (dbupdate->cli) {
        if (dbupdate->progress) {
          fprintf (stdout, "\r100.00\n");
        }
        if (dbupdate->verbose) {
          fprintf (stdout,
              "found %u skip %u indb %u new %u updated %u notaudio %u writetag %u\n",
              dbupdate->counts [C_FILE_COUNT],
              dbupdate->counts [C_FILE_SKIPPED],
              dbupdate->counts [C_IN_DB],
              dbupdate->counts [C_NEW],
              dbupdate->counts [C_UPDATED],
              dbupdate->counts [C_NON_AUDIO],
              dbupdate->counts [C_WRITE_TAGS]);
        }
        fflush (stdout);
      }
    }
  }

  if (dbupdate->state == DB_UPD_FINISH) {
    char  tbuff [MAXPATHLEN];
    char  dbfname [MAXPATHLEN];

    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
    pathbldMakePath (dbfname, sizeof (dbfname),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);

    if (dbupdate->newdatabase) {
      dbEndBatch (dbupdate->newmusicdb);
      dbClose (dbupdate->newmusicdb);
      dbupdate->newmusicdb = NULL;
      /* rename the database file */
      filemanipMove (tbuff, dbfname);
    } else {
      dbEndBatch (dbupdate->musicdb);
    }

    dbupdateOutputProgress (dbupdate);
    /* CONTEXT: database update: status message: total number of files found */
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Total Files"), dbupdate->counts [C_FILE_COUNT]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    if (! dbupdate->rebuild) {
      /* CONTEXT: database update: status message: files found in the database */
      snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Loaded from Database"), dbupdate->counts [C_IN_DB]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }
    /* CONTEXT: database update: status message: new files saved to the database */
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("New Files"), dbupdate->counts [C_NEW]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    if (! dbupdate->rebuild && ! dbupdate->writetags) {
      /* CONTEXT: database update: status message: number of files updated in the database */
      snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Updated"), dbupdate->counts [C_UPDATED]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }
    if (dbupdate->writetags) {
      /* re-use the 'Updated' label for write-tags */
      /* CONTEXT: database update: status message: number of files updated */
      snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Updated"), dbupdate->counts [C_WRITE_TAGS]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }
    /* CONTEXT: database update: status message: other files that cannot be processed */
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Other Files"),
        dbupdate->counts [C_BAD] + dbupdate->counts [C_NULL_DATA] +
        dbupdate->counts [C_NO_TAGS] + dbupdate->counts [C_NON_AUDIO] +
        dbupdate->counts [C_BDJ_SKIP] + dbupdate->counts [C_BDJ_OLD_DIR]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    if (dbupdate->stoprequest) {
      /* CONTEXT: database update: status message */
      msg = _("Stopped by User");
    } else {
      /* CONTEXT: database update: status message */
      msg = _("Complete");
    }
    snprintf (tbuff, sizeof (tbuff), "-- %s", msg);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    logMsg (LOG_DBG, LOG_IMPORTANT, "-- finish: %"PRId64" ms stop-req: %d",
        (int64_t) mstimeend (&dbupdate->starttm), dbupdate->stoprequest);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    found: %u", dbupdate->counts [C_FILE_COUNT]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  skipped: %u", dbupdate->counts [C_FILE_SKIPPED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     sent: %u", dbupdate->counts [C_FILE_SENT]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "processed: %u", dbupdate->counts [C_FILE_PROC]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    in-db: %u", dbupdate->counts [C_IN_DB]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      new: %u", dbupdate->counts [C_NEW]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  updated: %u", dbupdate->counts [C_UPDATED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "write-tag: %u", dbupdate->counts [C_WRITE_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      bad: %u", dbupdate->counts [C_BAD]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     null: %u", dbupdate->counts [C_NULL_DATA]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  no tags: %u", dbupdate->counts [C_NO_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "not-audio: %u", dbupdate->counts [C_NON_AUDIO]);
    logMsg (LOG_DBG, LOG_IMPORTANT, " bdj-skip: %u", dbupdate->counts [C_BDJ_SKIP]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  old-dir: %u", dbupdate->counts [C_BDJ_OLD_DIR]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "max-write: %"PRIu64, (uint64_t) dbupdate->maxWriteLen);

    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS, "END");
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_FINISH, NULL);
    connDisconnect (dbupdate->conn, ROUTE_MANAGEUI);

    progstateShutdownProcess (dbupdate->progstate);
    return stop;
  }

  if (gKillReceived) {
    progstateShutdownProcess (dbupdate->progstate);
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    gKillReceived = 0;
  }
  return stop;
}

static bool
dbupdateListeningCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           flags;

  logProcBegin (LOG_PROC, "dbupdateListeningCallback");

  flags = PROCUTIL_DETACH;
  if ((dbupdate->startflags & BDJ4_INIT_NO_DETACH) == BDJ4_INIT_NO_DETACH) {
    flags = PROCUTIL_NO_DETACH;
  }

  if ((dbupdate->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    dbupdate->processes [ROUTE_DBTAG] = procutilStartProcess (
        ROUTE_PLAYER, "bdj4dbtag", flags, NULL);
  }

  logProcEnd (LOG_PROC, "dbupdateListeningCallback", "");
  return STATE_FINISHED;
}

static bool
dbupdateConnectingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           c = 0;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "dbupdateConnectingCallback");

  connProcessUnconnected (dbupdate->conn);

  if ((dbupdate->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (dbupdate->conn, ROUTE_DBTAG)) {
      connConnect (dbupdate->conn, ROUTE_DBTAG);
    }
    if (! dbupdate->cli) {
      if (! connIsConnected (dbupdate->conn, ROUTE_MANAGEUI)) {
        connConnect (dbupdate->conn, ROUTE_MANAGEUI);
      }
    }
  }

  if (connIsConnected (dbupdate->conn, ROUTE_DBTAG)) {
    ++c;
  }
  if (connIsConnected (dbupdate->conn, ROUTE_MANAGEUI)) {
    ++c;
  }
  if (dbupdate->cli) {
    if (c == 1) { rc = STATE_FINISHED; }
  } else {
    if (c == 2) { rc = STATE_FINISHED; }
  }

  logProcEnd (LOG_PROC, "dbupdateConnectingCallback", "");
  return rc;
}

static bool
dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           c = 0;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "dbupdateHandshakeCallback");

  connProcessUnconnected (dbupdate->conn);

  if (connHaveHandshake (dbupdate->conn, ROUTE_DBTAG)) {
    ++c;
  }
  if (connHaveHandshake (dbupdate->conn, ROUTE_MANAGEUI)) {
    ++c;
  }
  if (dbupdate->cli) {
    if (c == 1) { rc = STATE_FINISHED; }
  } else {
    if (c == 2) { rc = STATE_FINISHED; }
  }

  logProcEnd (LOG_PROC, "dbupdateHandshakeCallback", "");
  return rc;
}

static bool
dbupdateStoppingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateStoppingCallback");

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, false);
  connDisconnect (dbupdate->conn, ROUTE_MANAGEUI);
  logProcEnd (LOG_PROC, "dbupdateStoppingCallback", "");
  return STATE_FINISHED;
}

static bool
dbupdateStopWaitCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t  *dbupdate = tdbupdate;
  bool        rc;

  rc = connWaitClosed (dbupdate->conn, &dbupdate->stopwaitcount);
  return rc;
}

static bool
dbupdateClosingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateClosingCallback");

  bdj4shutdown (ROUTE_DBUPDATE, dbupdate->musicdb);
  if (dbupdate->newmusicdb != NULL) {
    dbClose (dbupdate->newmusicdb);
  }

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, true);
  procutilFreeAll (dbupdate->processes);

  if (dbupdate->org != NULL) {
    orgFree (dbupdate->org);
  }
  if (dbupdate->badfnregex != NULL) {
    regexFree (dbupdate->badfnregex);
  }
  slistFree (dbupdate->fileList);

  logProcEnd (LOG_PROC, "dbupdateClosingCallback", "");
  return STATE_FINISHED;
}

static void
dbupdateProcessTagData (dbupdate_t *dbupdate, char *args)
{
  slist_t   *tagdata;
  char      *ffn;
  const char *dbfname;
  char      *data;
  char      *tokstr;
  slistidx_t orgiteridx;
  int       tagkey;
  dbidx_t   rrn;
  musicdb_t *currdb;
  song_t    *song = NULL;
  size_t    len;
  char      *val;
  int       rewrite;


  ffn = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  data = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);

  logMsg (LOG_DBG, LOG_DBUPDATE, "__ process %s", ffn);

  if (data == NULL) {
    /* complete failure */
    logMsg (LOG_DBG, LOG_DBUPDATE, "  null data");
    dbupdateIncCount (dbupdate, C_NULL_DATA);
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  tagdata = audiotagParseData (ffn, data, &rewrite);
  if (slistGetCount (tagdata) == 0) {
    /* if there is not even a duration, then file is no good */
    /* probably not an audio file */
    logMsg (LOG_DBG, LOG_DBUPDATE, "  no tags");
    dbupdateIncCount (dbupdate, C_NO_TAGS);
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  /* write-tags has its own processing */
  if (dbupdate->writetags) {
    dbupdateWriteTags (dbupdate, ffn, tagdata);
    slistFree (tagdata);
    return;
  }

  if (dbupdate->dancefromgenre) {
    val = slistGetStr (tagdata, tagdefs [TAG_DANCE].tag);
    if (val == NULL || ! *val) {
      val = slistGetStr (tagdata, tagdefs [TAG_GENRE].tag);
      if (val != NULL && *val) {
        slistSetStr (tagdata, tagdefs [TAG_DANCE].tag, val);
      }
    }
  }

  val = slistGetStr (tagdata, tagdefs [TAG_DANCELEVEL].tag);
  if (val == NULL || ! *val) {
    level_t *levels;

    levels = bdjvarsdfGet (BDJVDF_LEVELS);
    slistSetStr (tagdata, tagdefs [TAG_DANCELEVEL].tag,
        levelGetDefaultName (levels));
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

  /* use the regex to parse the filename and process */
  /* the data that is found there. */
  logMsg (LOG_DBG, LOG_DBUPDATE, "regex-parse:");
  orgStartIterator (dbupdate->org, &orgiteridx);
  while ((tagkey = orgIterateTagKey (dbupdate->org, &orgiteridx)) >= 0) {
    const char  *relfname;

    val = slistGetStr (tagdata, tagdefs [tagkey].tag);
    if (val != NULL && *val) {
      /* this tag already exists in the tagdata, keep it */
      logMsg (LOG_DBG, LOG_DBUPDATE, "  keep existing %s", tagdefs [tagkey].tag);
      continue;
    }

    /* use the relfname here, want the prefix stripped before doing */
    relfname = dbupdateGetRelativePath (dbupdate, ffn);
    /* a regex check */
    val = orgGetFromPath (dbupdate->org, relfname, tagkey);
    if (val != NULL && *val) {
      slistSetStr (tagdata, tagdefs [tagkey].tag, val);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  %s : %s", tagdefs [tagkey].tag, val);
    }
  }

  dbfname = ffn;
  if (dbupdate->usingmusicdir) {
    dbfname = dbupdateGetRelativePath (dbupdate, ffn);
  }
  rrn = MUSICDB_ENTRY_NEW;
  if (! dbupdate->rebuild) {
    song = dbGetByName (dbupdate->musicdb, dbfname);
    if (song != NULL) {
      rrn = songGetNum (song, TAG_RRN);
    }
  }

  /* the dbWrite() procedure will set the FILE tag */
  currdb = dbupdate->musicdb;
  if (dbupdate->newdatabase) {
    currdb = dbupdate->newmusicdb;
  }
  len = dbWrite (currdb, dbfname, tagdata, rrn);
  if (rrn == MUSICDB_ENTRY_NEW) {
    dbupdateIncCount (dbupdate, C_NEW);
  } else {
    dbupdateIncCount (dbupdate, C_UPDATED);
  }
  if (len > dbupdate->maxWriteLen) {
    dbupdate->maxWriteLen = len;
  }

  slistFree (tagdata);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
}

static void
dbupdateWriteTags (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata)
{
  const char  *dbfname;
  song_t      *song;
  slist_t     *newtaglist;

  if (ffn == NULL) {
    return;
  }

  dbfname = ffn;
  if (dbupdate->usingmusicdir) {
    dbfname = dbupdateGetRelativePath (dbupdate, ffn);
  }
  song = dbGetByName (dbupdate->musicdb, dbfname);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  newtaglist = songTagList (song);
  audiotagWriteTags (ffn, tagdata, newtaglist, 0, AT_UPDATE_MOD_TIME);
  slistFree (newtaglist);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
  dbupdateIncCount (dbupdate, C_WRITE_TAGS);
}

static void
dbupdateSigHandler (int sig)
{
  gKillReceived = 1;
}

static void
dbupdateOutputProgress (dbupdate_t *dbupdate)
{
  double    dval;
  char      tbuff [40];

  if (! dbupdate->progress) {
    return;
  }

  if (dbupdate->counts [C_FILE_COUNT] == 0) {
    if (dbupdate->cli) {
      fprintf (stdout, "\r  0.00");
      fflush (stdout);
    } else {
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS,
          "PROG 0.00");
    }
    return;
  }

  if (! mstimeCheck (&dbupdate->outputTimer)) {
    return;
  }

  mstimeset (&dbupdate->outputTimer, 50);

  /* files processed / filecount */
  dval = ((double) dbupdate->counts [C_FILE_PROC] +
      (double) dbupdate->counts [C_FILE_SKIPPED]) /
      (double) dbupdate->counts [C_FILE_COUNT];
  snprintf (tbuff, sizeof (tbuff), "PROG %.2f", dval);
  if (dbupdate->cli) {
    fprintf (stdout, "\r%6.2f", dval * 100.0);
    fflush (stdout);
  } else {
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS, tbuff);
  }
}

static const char *
dbupdateGetRelativePath (dbupdate_t *dbupdate, const char *fn)
{
  const char  *p;

  p = fn;
  if (strncmp (fn, dbupdate->dbtopdir, dbupdate->dbtopdirlen) == 0) {
    p += dbupdate->musicdirlen + 1;
  }

  return p;
}

static bool
checkOldDirList (dbupdate_t *dbupdate, const char *fn)
{
  const char  *fp;
  char        *olddirs;
  char        *p;
  char        *tokstr;
  size_t      len;

  fp = fn + dbupdate->dbtopdirlen + 1;
  olddirs = strdup (dbupdate->olddirlist);
  p = strtok_r (olddirs, ";", &tokstr);
  while (p != NULL) {
    len = strlen (p);
    if (strlen (fp) > len &&
        strncmp (p, fp, len) == 0 &&
        *(fp + len) == '/') {
      return true;
    }
    p = strtok_r (NULL, ";", &tokstr);
  }
  free (olddirs);
  return false;
}

static void
dbupdateIncCount (dbupdate_t *dbupdate, int tag)
{
  ++dbupdate->counts [tag];
}

