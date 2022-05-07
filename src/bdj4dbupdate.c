/*
 *  bdj4dbupdate
 *  updates the database.
 *  there are various modes.
 *    - rebuild
 *      rebuild and replace the database in its entirety.
 *    - check-for-new
 *      check for new files and changes and add them.
 *    - updfromtags
 *      update db from tags in audio files.
 *      this is the same as checknew, except that all audio files tags
 *      are loaded and updated in the database.
 *      so the processing is similar to a rebuild, but using the
 *      existing database and updating the records in the database.
 *    - writetags
 *      write db tags to the audio files
 *    - reorganize
 *      use the organization settings to reorg the files.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

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
#include "fileop.h"
#include "filemanip.h"
#include "level.h"
#include "log.h"
#include "musicdb.h"
#include "orgutil.h"
#include "osutils.h"
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

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  int               startflags;
  int               state;
  musicdb_t         *musicdb;
  musicdb_t         *nmusicdb;
  char              *musicdir;
  size_t            musicdirlen;
  mstime_t          outputTimer;
  org_t             *org;
  slist_t           *fileList;
  slistidx_t        filelistIterIdx;
  bdjregex_t        *badfnregex;
  dbidx_t           fileCount;            // total from reading dir
  dbidx_t           filesSkipped;         // any sort of skip
  dbidx_t           filesSent;
  dbidx_t           filesReceived;
  dbidx_t           filesProcessed;       // processed + skipped = count
  dbidx_t           countInDatabase;
  dbidx_t           countNew;
  dbidx_t           countUpdated;
  dbidx_t           countBad;
  dbidx_t           countNonAudio;
  dbidx_t           countNullData;
  dbidx_t           countNoTags;
  mstime_t          starttm;
  int               stopwaitcount;
  bool              rebuild : 1;
  bool              checknew : 1;
  bool              progress : 1;
  bool              updfromtags : 1;
  bool              writetags : 1;
  bool              reorganize : 1;
  bool              newdatabase : 1;
} dbupdate_t;

#define FNAMES_SENT_PER_ITER  30

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
static void     dbupdateSigHandler (int sig);
static void     dbupdateOutputProgress (dbupdate_t *dbupdate);
static char *   dbupdateGetRelativePath (dbupdate_t *dbupdate, char *fn);

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
  dbupdate.nmusicdb = NULL;
  dbupdate.filesProcessed = 0;
  dbupdate.fileCount = 0;
  dbupdate.filesSkipped = 0;
  dbupdate.filesSent = 0;
  dbupdate.filesReceived = 0;
  dbupdate.countInDatabase = 0;
  dbupdate.countNew = 0;
  dbupdate.countBad = 0;
  dbupdate.countNonAudio = 0;
  dbupdate.countNullData = 0;
  dbupdate.countNoTags = 0;
  dbupdate.countUpdated = 0;
  dbupdate.stopwaitcount = 0;
  dbupdate.rebuild = false;
  dbupdate.checknew = false;
  dbupdate.progress = false;
  dbupdate.updfromtags = false;
  dbupdate.writetags = false;
  dbupdate.reorganize = false;
  dbupdate.newdatabase = false;
  mstimeset (&dbupdate.outputTimer, 0);
  dbupdate.org = NULL;

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

  dbupdate.org = orgAlloc (bdjoptGetStr (OPT_G_AO_PATHFMT));

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
  if ((dbupdate.startflags & BDJ4_DB_PROGRESS) == BDJ4_DB_PROGRESS) {
    dbupdate.progress = true;
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
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          progstateShutdownProcess (dbupdate->progstate);
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
  int           stop = false;


  if (! progstateIsRunning (dbupdate->progstate)) {
    progstateProcess (dbupdate->progstate);
    if (progstateCurrState (dbupdate->progstate) == STATE_CLOSED) {
      stop = true;
    }
    if (gKillReceived) {
      progstateShutdownProcess (dbupdate->progstate);
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return stop;
  }

  connProcessUnconnected (dbupdate->conn);

  if (dbupdate->state == DB_UPD_INIT) {
    char  dbfname [MAXPATHLEN];

    if (dbupdate->rebuild) {
      dbupdate->newdatabase = true;
    }

    pathbldMakePath (dbfname, sizeof (dbfname),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
    filemanipBackup (dbfname, 4);

    if (dbupdate->newdatabase) {
      char  tbuff [MAXPATHLEN];

      pathbldMakePath (tbuff, sizeof (tbuff),
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DATA);
      fileopDelete (tbuff);
      dbupdate->nmusicdb = dbOpen (tbuff);
      assert (dbupdate->nmusicdb != NULL);
      dbStartBatch (dbupdate->nmusicdb);
    } else {
      dbStartBatch (dbupdate->musicdb);
    }
    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    char  tbuff [100];

    mstimestart (&dbupdate->starttm);

    dbupdateOutputProgress (dbupdate);

    dbupdate->musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->musicdirlen = strlen (dbupdate->musicdir);
    dbupdate->fileList = filemanipRecursiveDirList (dbupdate->musicdir, FILEMANIP_FILES);

    dbupdate->fileCount = slistGetCount (dbupdate->fileList);
    mstimeend (&dbupdate->starttm);
    logMsg (LOG_DBG, LOG_IMPORTANT, "read directory %s: %ld ms",
        dbupdate->musicdir, mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "  %d files found", dbupdate->fileCount);

    /* message to manageui */
    snprintf (tbuff, sizeof (tbuff), _("%d files found"), dbupdate->fileCount);
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
        ++dbupdate->filesSkipped;
        ++dbupdate->countNonAudio;
        continue;
      }
      pathInfoFree (pi);

      /* check to see if the audio file is already in the database */
      /* this is done for all modes except for rebuild */
      /* 'checknew' skips any processing for an audio file */
      /* that is already present */
      if (! dbupdate->rebuild && fn != NULL) {
        char  *p;

        p = dbupdateGetRelativePath (dbupdate, fn);
        if (dbGetByName (dbupdate->musicdb, p) != NULL) {
          logMsg (LOG_DBG, LOG_DBUPDATE, "  in-database");
          ++dbupdate->countInDatabase;

          /* if doing a checknew, no need for further processing */
          /* the file exists, don't change the file or the database */
          if (dbupdate->checknew) {
            ++dbupdate->filesSkipped;
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
        ++dbupdate->filesSkipped;
        ++dbupdate->countBad;
        continue;
      }

      connSendMessage (dbupdate->conn, ROUTE_DBTAG, MSG_DB_FILE_CHK, fn);
      ++dbupdate->filesSent;
      ++count;
      if (count > FNAMES_SENT_PER_ITER) {
        break;
      }
    }

    if (fn == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- skipped (%d)", dbupdate->filesSkipped);
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- all filenames sent (%d): %ld ms",
          dbupdate->filesSent, mstimeend (&dbupdate->starttm));
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_SEND ||
      dbupdate->state == DB_UPD_PROCESS) {
    dbupdateOutputProgress (dbupdate);
    logMsg (LOG_DBG, LOG_DBUPDATE, "progress: %ld+%ld(%ld) >= %ld\n",
        dbupdate->filesProcessed, dbupdate->filesSkipped,
        dbupdate->filesProcessed + dbupdate->filesSkipped,
        dbupdate->fileCount);
    if (dbupdate->filesProcessed + dbupdate->filesSkipped >=
        dbupdate->fileCount) {
      dbupdate->state = DB_UPD_FINISH;
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
      dbEndBatch (dbupdate->nmusicdb);
      dbClose (dbupdate->nmusicdb);
      dbupdate->nmusicdb = NULL;
      /* rename the database file */
      filemanipMove (tbuff, dbfname);
    } else {
      dbEndBatch (dbupdate->musicdb);
    }

    dbupdateOutputProgress (dbupdate);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG,
        _("Complete"));
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Total Files"), dbupdate->fileCount);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Loaded from Database"), dbupdate->countInDatabase);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("New Files"), dbupdate->countNew);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Updated"), dbupdate->countUpdated);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Other Files"),
        dbupdate->countBad + dbupdate->countNullData +
        dbupdate->countNoTags + dbupdate->countNonAudio);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    logMsg (LOG_DBG, LOG_IMPORTANT, "-- finish: %ld ms",
        mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "    found: %u", dbupdate->fileCount);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  skipped: %u", dbupdate->filesSkipped);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     sent: %u", dbupdate->filesSent);
    logMsg (LOG_DBG, LOG_IMPORTANT, "processed: %u", dbupdate->filesProcessed);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    in-db: %u", dbupdate->countInDatabase);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      new: %u", dbupdate->countNew);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  updated: %u", dbupdate->countUpdated);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      bad: %u", dbupdate->countBad);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     null: %u", dbupdate->countNullData);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  no tags: %u", dbupdate->countNoTags);
    logMsg (LOG_DBG, LOG_IMPORTANT, "not-audio: %u", dbupdate->countNonAudio);

    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS, "END");
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_FINISH, NULL);
    connDisconnect (dbupdate->conn, ROUTE_MANAGEUI);

    progstateShutdownProcess (dbupdate->progstate);
    return stop;
  }

  if (gKillReceived) {
    progstateShutdownProcess (dbupdate->progstate);
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
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
  return true;
}

static bool
dbupdateConnectingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           c = 0;
  bool          rc = false;

  logProcBegin (LOG_PROC, "dbupdateConnectingCallback");

  connProcessUnconnected (dbupdate->conn);

  if ((dbupdate->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! connIsConnected (dbupdate->conn, ROUTE_DBTAG)) {
      connConnect (dbupdate->conn, ROUTE_DBTAG);
    }
    if ((dbupdate->startflags & BDJ4_CLI) != BDJ4_CLI) {
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
  if ((dbupdate->startflags & BDJ4_CLI) == BDJ4_CLI) {
    if (c == 1) { rc = true; }
  } else {
    if (c == 2) { rc = true; }
  }

  logProcEnd (LOG_PROC, "dbupdateConnectingCallback", "");
  return rc;
}

static bool
dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  int           c = 0;
  bool          rc = false;

  logProcBegin (LOG_PROC, "dbupdateHandshakeCallback");

  connProcessUnconnected (dbupdate->conn);

  if (connHaveHandshake (dbupdate->conn, ROUTE_DBTAG)) {
    ++c;
  }
  if (connHaveHandshake (dbupdate->conn, ROUTE_MANAGEUI)) {
    ++c;
  }
  if ((dbupdate->startflags & BDJ4_CLI) == BDJ4_CLI) {
    if (c == 1) { rc = true; }
  } else {
    if (c == 2) { rc = true; }
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
  return true;
}

static bool
dbupdateStopWaitCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t  *dbupdate = tdbupdate;
  bool        rc = false;

  logProcBegin (LOG_PROC, "dbupdateStopWaitCallback");

  rc = connCheckAll (dbupdate->conn);
  if (rc == false) {
    ++dbupdate->stopwaitcount;
    if (dbupdate->stopwaitcount > STOP_WAIT_COUNT_MAX) {
      rc = true;
    }
  }

  if (rc) {
    connDisconnectAll (dbupdate->conn);
  }

  logProcEnd (LOG_PROC, "dbupdateStopWaitCallback", "");
  return rc;
}

static bool
dbupdateClosingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateClosingCallback");

  bdj4shutdown (ROUTE_DBUPDATE, dbupdate->musicdb);
  if (dbupdate->nmusicdb != NULL) {
    dbClose (dbupdate->nmusicdb);
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
  slistidx_t orgiteridx;
  int       tagkey;
  char      *p;
  dbidx_t   rrn;
  musicdb_t *currdb;
  song_t    *song = NULL;


  ffn = strtok_r (args, MSG_ARGS_RS_STR, &tokstr);
  data = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);

  logMsg (LOG_DBG, LOG_DBUPDATE, "__ process %s\n", ffn);

  if (data == NULL) {
    /* complete failure */
    logMsg (LOG_DBG, LOG_DBUPDATE, "  null data");
    ++dbupdate->countNullData;
    ++dbupdate->filesProcessed;
    return;
  }

  tagdata = audiotagParseData (ffn, data);
  if (slistGetCount (tagdata) == 0) {
    /* if there is not even a duration, then file is no good */
    /* probably not an audio file */
    logMsg (LOG_DBG, LOG_DBUPDATE, "  no tags");
    ++dbupdate->countNoTags;
    ++dbupdate->filesProcessed;
    return;
  }

  if (strncmp (ffn, dbupdate->musicdir, dbupdate->musicdirlen) == 0) {
    relfname = ffn + dbupdate->musicdirlen + 1;
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

  /* unfortunately, this slows the database rebuild down ...*/
  /* use the regex to parse the filename and process */
  /* the data that is found there. */
  logMsg (LOG_DBG, LOG_DBUPDATE, "regex-parse:");
  orgStartIterator (dbupdate->org, &orgiteridx);
  while ((tagkey = orgIterateTagKey (dbupdate->org, &orgiteridx)) >= 0) {
    char  *val;

    val = slistGetStr (tagdata, tagdefs [tagkey].tag);
    if (val != NULL && *val) {
      /* this tag already exists in the tagdata, keep it */
      logMsg (LOG_DBG, LOG_DBUPDATE, "  keep existing %s", tagdefs [tagkey].tag);
      continue;
    }

    val = orgGetFromPath (dbupdate->org, relfname, tagkey);
    if (val != NULL && *val) {
      slistSetStr (tagdata, tagdefs [tagkey].tag, val);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  %s : %s", tagdefs [tagkey].tag, val);
    }
  }

  rrn = MUSICDB_ENTRY_NEW;
  if (! dbupdate->rebuild) {
    p = dbupdateGetRelativePath (dbupdate, ffn);
    song = dbGetByName (dbupdate->musicdb, p);
    if (song != NULL) {
      rrn = songGetNum (song, TAG_RRN);
    }
  }

  /* the dbWrite() procedure will set the FILE tag */
  relfname = slistGetStr (dbupdate->fileList, ffn);
  currdb = dbupdate->musicdb;
  if (dbupdate->newdatabase) {
    currdb = dbupdate->nmusicdb;
  }
  dbWrite (currdb, relfname, tagdata, rrn);
  if (rrn == MUSICDB_ENTRY_NEW) {
    ++dbupdate->countNew;
  } else {
    ++dbupdate->countUpdated;
  }

  slistFree (tagdata);
  ++dbupdate->filesProcessed;
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

  if (dbupdate->fileCount == 0) {
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS,
        "PROG 0.00");
    return;
  }

  if (! mstimeCheck (&dbupdate->outputTimer)) {
    return;
  }

  mstimeset (&dbupdate->outputTimer, 50);

  /* files processed / filecount */
  dval = ((double) dbupdate->filesProcessed +
      (double) dbupdate->filesSkipped) /
      (double) dbupdate->fileCount;
  snprintf (tbuff, sizeof (tbuff), "PROG %.2f", dval);
  connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_PROGRESS, tbuff);
}

static char *
dbupdateGetRelativePath (dbupdate_t *dbupdate, char *fn)
{
  char    *p;

  p = fn;
  if (strncmp (fn, dbupdate->musicdir, dbupdate->musicdirlen) == 0) {
    p += dbupdate->musicdirlen + 1;
  }

  return p;
}
