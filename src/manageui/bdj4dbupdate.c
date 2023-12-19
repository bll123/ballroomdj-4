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
 *    - compact
 *      create a new db file, bypass any deleted entries.
 *      may be used in conjunction w/check-for-new.
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
#include <signal.h>

#include "audiofile.h"
#include "audiosrc.h"
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
#include "itunes.h"
#include "level.h"
#include "log.h"
#include "mdebug.h"
#include "musicdb.h"
#include "orgopt.h"
#include "orgutil.h"
#include "ossignal.h"
#include "pathbld.h"
#include "pathutil.h"
#include "progstate.h"
#include "procutil.h"
#include "queue.h"
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
  DB_UPD_PROC_FN,
  DB_UPD_PROCESS,
  DB_UPD_FINISH,
};

enum {
  C_FILE_COUNT,
  C_FILE_PROC,
  C_FILE_SKIPPED,
  C_FILE_QUEUED,
  C_QUEUE_MAX,
  C_IN_DB,
  C_NEW,
  C_UPDATED,
  C_WRITE_TAGS,
  C_RENAMED,
  C_CANNOT_RENAME,
  C_BAD,
  C_NON_AUDIO,
  C_ORIG_SKIP,
  C_DEL_SKIP,
  C_BDJ_OLD_DIR,
  C_NULL_DATA,
  C_NO_TAGS,
  C_MAX,
};

typedef struct {
  char        *ffn;
  char        *data;
} tagdataitem_t;

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  long              startflags;
  int               state;
  musicdb_t         *musicdb;
  musicdb_t         *newmusicdb;
  const char        *musicdir;
  size_t            musicdirlen;
  const char        *dbtopdir;
  int               prefixlen;
  size_t            dbtopdirlen;
  mstime_t          outputTimer;
  org_t             *org;
  org_t             *orgold;
  asiter_t          *asiter;
  bdjregex_t        *badfnregex;
  dbidx_t           counts [C_MAX];
  size_t            maxWriteLen;
  mstime_t          starttm;
  int               stopwaitcount;
  const char        *olddirlist;
  itunes_t          *itunes;
  queue_t           *tagdataq;
  /* base database operations */
  bool              checknew : 1;
  bool              compact : 1;
  bool              rebuild : 1;
  bool              reorganize : 1;
  bool              updfromitunes : 1;
  bool              updfromtags : 1;
  bool              writetags : 1;
  /* database handling */
  bool              cleandatabase : 1;
  /* other stuff */
  bool              cli : 1;
  bool              dancefromgenre : 1;
  bool              haveolddirlist : 1;
  bool              progress : 1;
  bool              stoprequest : 1;
  bool              usingmusicdir : 1;
  bool              verbose : 1;
} dbupdate_t;

enum {
  FNAMES_SENT_PER_ITER = 30,
  QUEUE_PROCESS_LIMIT = 30,
};

static int      dbupdateProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      dbupdateProcessing (void *udata);
static bool     dbupdateConnectingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateStoppingCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateStopWaitCallback (void *tdbupdate, programstate_t programState);
static bool     dbupdateClosingCallback (void *tdbupdate, programstate_t programState);
static void     dbupdateQueueFile (dbupdate_t *dbupdate, const char *args);
static void     dbupdateTagDataFree (void *data);
static void     dbupdateProcessFileQueue (dbupdate_t *dbupdate);
static void     dbupdateProcessFile (dbupdate_t *dbupdate, const char *ffn, char *data);
static void     dbupdateWriteTags (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata);
static void     dbupdateFromiTunes (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata);
static void     dbupdateReorganize (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata);
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
  char          *p;
  const char    *topt;

#if BDJ4_MEM_DEBUG
  mdebugInit ("dbup");
#endif

  dbupdate.state = DB_UPD_INIT;
  dbupdate.musicdb = NULL;
  dbupdate.newmusicdb = NULL;
  dbupdate.asiter = NULL;
  for (int i = 0; i < C_MAX; ++i) {
    dbupdate.counts [i] = 0;
  }
  dbupdate.maxWriteLen = 0;
  dbupdate.stopwaitcount = 0;
  dbupdate.itunes = NULL;
  dbupdate.tagdataq = queueAlloc ("tagdata-q", dbupdateTagDataFree);
  dbupdate.org = NULL;
  dbupdate.orgold = NULL;
  dbupdate.checknew = false;
  dbupdate.compact = false;
  dbupdate.rebuild = false;
  dbupdate.reorganize = false;
  dbupdate.updfromitunes = false;
  dbupdate.updfromtags = false;
  dbupdate.writetags = false;
  dbupdate.cleandatabase = false;
  dbupdate.cli = false;
  dbupdate.dancefromgenre = false;
  dbupdate.haveolddirlist = false;
  dbupdate.olddirlist = NULL;
  dbupdate.progress = false;
  dbupdate.stoprequest = false;
  dbupdate.usingmusicdir = true;
  dbupdate.verbose = false;
  mstimeset (&dbupdate.outputTimer, 0);

  dbupdate.dancefromgenre = bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE);

  dbupdate.progstate = progstateInit ("dbupdate");
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

  dbupdate.startflags = BDJ4_INIT_ALL;
  bdj4startup (argc, argv, &dbupdate.musicdb,
      "dbup", ROUTE_DBUPDATE, &dbupdate.startflags);
  logProcBegin (LOG_PROC, "dbupdate");

  dbupdate.olddirlist = bdjoptGetStr (OPT_M_DIR_OLD_SKIP);
  if (dbupdate.olddirlist != NULL && *dbupdate.olddirlist) {
    dbupdate.haveolddirlist = true;
  }

  topt = bdjoptGetStr (OPT_G_ORGPATH);
  dbupdate.org = orgAlloc (topt);
  if (strstr (topt, "BYPASS") != NULL) {
    dbupdate.orgold = orgAlloc (bdjoptGetStr (OPT_G_OLDORGPATH));
  }

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
  if ((dbupdate.startflags & BDJ4_DB_COMPACT) == BDJ4_DB_COMPACT) {
    dbupdate.compact = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_REBUILD) == BDJ4_DB_REBUILD) {
    dbupdate.rebuild = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_UPD_FROM_TAGS) == BDJ4_DB_UPD_FROM_TAGS) {
    dbupdate.updfromtags = true;
  }
  if ((dbupdate.startflags & BDJ4_DB_UPD_FROM_ITUNES) == BDJ4_DB_UPD_FROM_ITUNES) {
    dbupdate.updfromitunes = true;
    dbupdate.itunes = itunesAlloc ();
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
#if BDJ4_MEM_DEBUG
  mdebugReport ();
  mdebugCleanup ();
#endif
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
          logMsg (LOG_DBG, LOG_INFO, "stop request received");
          dbupdate->state = DB_UPD_FINISH;
          dbupdate->stoprequest = true;
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
      dbupdate->cleandatabase = true;
    }
    if (dbupdate->compact) {
      dbupdate->cleandatabase = true;
    }

    dbBackup ();

    if (dbupdate->cleandatabase) {
      char  tbuff [MAXPATHLEN];

      pathbldMakePath (tbuff, sizeof (tbuff),
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
      fileopDelete (tbuff);
      dbupdate->newmusicdb = dbOpen (tbuff);
      dbStartBatch (dbupdate->newmusicdb);
    }

    dbStartBatch (dbupdate->musicdb);

    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    char  tbuff [100];
    char  *tstr;

    mstimestart (&dbupdate->starttm);

    dbupdateOutputProgress (dbupdate);

    dbupdate->prefixlen = 0;
    dbupdate->usingmusicdir = true;
    dbupdate->musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->musicdirlen = strlen (dbupdate->musicdir);
    dbupdate->dbtopdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    tstr = bdjvarsGetStr (BDJV_DB_TOP_DIR);
    if (strcmp (tstr, dbupdate->musicdir) != 0) {
      dbupdate->usingmusicdir = false;
      dbupdate->dbtopdir = tstr;
      dbupdate->prefixlen = strlen (tstr) + 1;  // include the slash
    }
    dbupdate->dbtopdirlen = strlen (dbupdate->dbtopdir);

    logMsg (LOG_DBG, LOG_BASIC, "dbtopdir %s", dbupdate->dbtopdir);
    dbupdate->asiter = audiosrcStartIterator (dbupdate->dbtopdir);

    dbupdate->counts [C_FILE_COUNT] = audiosrcIterCount (dbupdate->asiter);
    mstimeend (&dbupdate->starttm);
    logMsg (LOG_DBG, LOG_IMPORTANT, "read directory %s: %" PRId64 " ms",
        dbupdate->dbtopdir, (int64_t) mstimeend (&dbupdate->starttm));
    logMsg (LOG_DBG, LOG_IMPORTANT, "  %u files found", dbupdate->counts [C_FILE_COUNT]);

    /* message to manageui */
    /* CONTEXT: database update: status message */
    snprintf (tbuff, sizeof (tbuff), _("%d files found"), dbupdate->counts [C_FILE_COUNT]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    dbupdate->state = DB_UPD_PROC_FN;
  }

  /* need to check the queue count so that too much memory is not used */
  /* all at once */
  if (dbupdate->state == DB_UPD_PROC_FN &&
      queueGetCount (dbupdate->tagdataq) <= QUEUE_PROCESS_LIMIT) {
    const char  *fn;
    pathinfo_t  *pi;

    while ((fn = audiosrcIterate (dbupdate->asiter)) != NULL) {
      song_t    *song;

      if (audiosrcGetType (fn) != AUDIOSRC_TYPE_FILE) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_NON_AUDIO);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  audsrc-skip-not-file");
        continue;
      }

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
      if (pathInfoExtCheck (pi, BDJ4_GENERIC_ORIG_EXT) ||
          pathInfoExtCheck (pi, bdjvarsGetStr (BDJV_ORIGINAL_EXT))) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_ORIG_SKIP);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-orig");
        pathInfoFree (pi);
        continue;
      }
      if (strncmp (pi->filename, bdjvarsGetStr (BDJV_DELETE_PFX),
          bdjvarsGetNum (BDJVL_DELETE_PFX_LEN)) == 0) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_DEL_SKIP);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-del");
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
      /* that is already present unless the compact flag is on */
      if (! dbupdate->rebuild && fn != NULL) {
        const char  *tsongfn;

        tsongfn = fn;
        if (dbupdate->usingmusicdir) {
          tsongfn = dbupdateGetRelativePath (dbupdate, fn);
        }
        song = dbGetByName (dbupdate->musicdb, tsongfn);
        if (song != NULL) {
          dbupdateIncCount (dbupdate, C_IN_DB);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  in-database (%u) ", dbupdate->counts [C_IN_DB]);

          /* if doing a checknew, no need for further processing */
          /* if doing a compact, the information must be written to */
          /* the new database. */
          /* the file exists, don't change the file or the database */
          if (dbupdate->checknew || dbupdate->compact) {
            if (dbupdate->checknew) {
              dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
            }
            if (dbupdate->compact) {
              slist_t   *tagdata;

              dbupdateIncCount (dbupdate, C_FILE_PROC);
              dbupdateIncCount (dbupdate, C_UPDATED);
              tagdata = songTagList (song);
              dbWrite (dbupdate->newmusicdb, tsongfn, tagdata, MUSICDB_ENTRY_NEW);
              slistFree (tagdata);
            }

            dbupdateOutputProgress (dbupdate);
            if (queueGetCount (dbupdate->tagdataq) >= QUEUE_PROCESS_LIMIT) {
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

      dbupdateQueueFile (dbupdate, fn);

      dbupdateIncCount (dbupdate, C_FILE_QUEUED);
      if (queueGetCount (dbupdate->tagdataq) >= QUEUE_PROCESS_LIMIT) {
        break;
      }
    }

    if (fn == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- skipped (%u)", dbupdate->counts [C_FILE_SKIPPED]);
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- all filenames sent (%u): %" PRId64 " ms",
          dbupdate->counts [C_FILE_QUEUED], (int64_t) mstimeend (&dbupdate->starttm));
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_PROC_FN ||
      dbupdate->state == DB_UPD_PROCESS) {
    dbupdateProcessFileQueue (dbupdate);
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
              "found %u skip %u indb %u new %u updated %u renamed %u norename %u notaudio %u writetag %u\n",
              dbupdate->counts [C_FILE_COUNT],
              dbupdate->counts [C_FILE_SKIPPED],
              dbupdate->counts [C_IN_DB],
              dbupdate->counts [C_NEW],
              dbupdate->counts [C_UPDATED],
              dbupdate->counts [C_RENAMED],
              dbupdate->counts [C_CANNOT_RENAME],
              dbupdate->counts [C_NON_AUDIO],
              dbupdate->counts [C_WRITE_TAGS]);
        }
        fflush (stdout);
      } /* if command line interface (testing) */
    } else {
      /* not done */
      if (queueGetCount (dbupdate->tagdataq) <= 0) {
        logMsg (LOG_DBG, LOG_DBUPDATE, "  claim not done, q-count <= 0");
        dbupdate->state = DB_UPD_PROC_FN;
      }
    } /* not done */
  } /* send or process */

  if (dbupdate->state == DB_UPD_FINISH) {
    char  tbuff [MAXPATHLEN];
    char  dbfname [MAXPATHLEN];

    pathbldMakePath (tbuff, sizeof (tbuff),
        MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
    pathbldMakePath (dbfname, sizeof (dbfname),
        MUSICDB_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);

    dbEndBatch (dbupdate->musicdb);

    if (dbupdate->cleandatabase) {
      dbEndBatch (dbupdate->newmusicdb);
      dbClose (dbupdate->newmusicdb);
      dbupdate->newmusicdb = NULL;
      /* the old db must be closed for the rename on windows */
      dbClose (dbupdate->musicdb);
      dbupdate->musicdb = NULL;
      /* rename the database file */
      filemanipMove (tbuff, dbfname);
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

    if (dbupdate->reorganize) {
      /* CONTEXT: database update: status message: number of files renamed */
      snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Renamed"), dbupdate->counts [C_RENAMED]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
      if (dbupdate->counts [C_CANNOT_RENAME] > 0) {
        /* CONTEXT: database update: status message: number of files that cannot be renamed */
        snprintf (tbuff, sizeof (tbuff), "%s : %u", _("Cannot Rename"), dbupdate->counts [C_CANNOT_RENAME]);
        connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
      }
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
        dbupdate->counts [C_BDJ_OLD_DIR] + dbupdate->counts [C_DEL_SKIP]);
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

    logMsg (LOG_DBG, LOG_IMPORTANT, "-- finish: %" PRId64 " ms stop-req: %d",
        (int64_t) mstimeend (&dbupdate->starttm), dbupdate->stoprequest);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    found: %u", dbupdate->counts [C_FILE_COUNT]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  skipped: %u", dbupdate->counts [C_FILE_SKIPPED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "   queued: %u", dbupdate->counts [C_FILE_QUEUED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "processed: %u", dbupdate->counts [C_FILE_PROC]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "queue-max: %u", dbupdate->counts [C_QUEUE_MAX]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    in-db: %u", dbupdate->counts [C_IN_DB]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      new: %u", dbupdate->counts [C_NEW]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  updated: %u", dbupdate->counts [C_UPDATED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  renamed: %u", dbupdate->counts [C_RENAMED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "no rename: %u", dbupdate->counts [C_CANNOT_RENAME]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "write-tag: %u", dbupdate->counts [C_WRITE_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      bad: %u", dbupdate->counts [C_BAD]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "     null: %u", dbupdate->counts [C_NULL_DATA]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  no tags: %u", dbupdate->counts [C_NO_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "not-audio: %u", dbupdate->counts [C_NON_AUDIO]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "orig-skip: %u", dbupdate->counts [C_ORIG_SKIP]);
    logMsg (LOG_DBG, LOG_IMPORTANT, " del-skip: %u", dbupdate->counts [C_DEL_SKIP]);
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
dbupdateConnectingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "dbupdateConnectingCallback");

  connProcessUnconnected (dbupdate->conn);

  if ((dbupdate->startflags & BDJ4_INIT_NO_START) != BDJ4_INIT_NO_START) {
    if (! dbupdate->cli &&
        ! connIsConnected (dbupdate->conn, ROUTE_MANAGEUI)) {
      connConnect (dbupdate->conn, ROUTE_MANAGEUI);
    }
  }

  if (dbupdate->cli ||
      connIsConnected (dbupdate->conn, ROUTE_MANAGEUI)) {
    rc = STATE_FINISHED;
  }

  logProcEnd (LOG_PROC, "dbupdateConnectingCallback", "");
  return rc;
}

static bool
dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin (LOG_PROC, "dbupdateHandshakeCallback");

  connProcessUnconnected (dbupdate->conn);

  if (dbupdate->cli ||
      connHaveHandshake (dbupdate->conn, ROUTE_MANAGEUI)) {
    rc = STATE_FINISHED;
  }

  if (rc == STATE_FINISHED && dbupdate->updfromitunes) {
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_WAIT, NULL);
    itunesParse (dbupdate->itunes);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_WAIT_FINISH, NULL);
  }

  logProcEnd (LOG_PROC, "dbupdateHandshakeCallback", "");
  return rc;
}

static bool
dbupdateStoppingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin (LOG_PROC, "dbupdateStoppingCallback");

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, PROCUTIL_NORM_TERM);
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
  dbClose (dbupdate->newmusicdb);

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (dbupdate->processes);

  itunesFree (dbupdate->itunes);
  orgFree (dbupdate->org);
  orgFree (dbupdate->orgold);
  regexFree (dbupdate->badfnregex);
  audiosrcCleanIterator (dbupdate->asiter);
  queueFree (dbupdate->tagdataq);

  logProcEnd (LOG_PROC, "dbupdateClosingCallback", "");
  return STATE_FINISHED;
}

static void
dbupdateQueueFile (dbupdate_t *dbupdate, const char *args)
{
  char          *ffn;
  char          *data;
  tagdataitem_t *tdi;
  char          *tokstr;
  dbidx_t       count;
  char          *targs;

  if (dbupdate->stoprequest) {
    /* crashes below (strdup (ffn)) if the requests are processed after */
    /* receiving a stop-request */
    /* do not know why exactly */
    return;
  }

  targs = mdstrdup (args);
  ffn = strtok_r (targs, MSG_ARGS_RS_STR, &tokstr);
  data = strtok_r (NULL, MSG_ARGS_RS_STR, &tokstr);

  tdi = mdmalloc (sizeof (tagdataitem_t));
  tdi->ffn = mdstrdup (ffn);
  tdi->data = NULL;
  if (data != NULL) {
    tdi->data = mdstrdup (data);
  }
  queuePush (dbupdate->tagdataq, tdi);
  count = queueGetCount (dbupdate->tagdataq);
  // fprintf (stderr, "q-push: %s %d\n", ffn, count);
  if (count > dbupdate->counts [C_QUEUE_MAX]) {
    dbupdate->counts [C_QUEUE_MAX] = count;
  }
  mdfree (targs);
}

static void
dbupdateTagDataFree (void *data)
{
  tagdataitem_t   *tdi = data;

  if (tdi != NULL) {
    dataFree (tdi->ffn);
    dataFree (tdi->data);
    mdfree (tdi);
  }
}

static void
dbupdateProcessFileQueue (dbupdate_t *dbupdate)
{
  tagdataitem_t *tdi;

  if (queueGetCount (dbupdate->tagdataq) <= 0) {
    return;
  }

  tdi = queuePop (dbupdate->tagdataq);
  // fprintf (stderr, "  pop: %s\n", tdi->ffn);
  dbupdateProcessFile (dbupdate, tdi->ffn, tdi->data);
  dbupdateTagDataFree (tdi);
}

static void
dbupdateProcessFile (dbupdate_t *dbupdate, const char *ffn, char *data)
{
  slist_t     *tagdata;
  const char  *songfname;
  slistidx_t  orgiteridx;
  int         tagkey;
  dbidx_t     rrn;
  musicdb_t   *currdb = NULL;
  song_t      *song = NULL;
  size_t      len;
  const char  *relfname;
  const char  *val;
  int         rewrite;

  logMsg (LOG_DBG, LOG_DBUPDATE, "__ process %s", ffn);

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

  /* update-from-itunes has its own processing */
  if (dbupdate->updfromitunes) {
    dbupdateFromiTunes (dbupdate, ffn, tagdata);
    slistFree (tagdata);
    return;
  }

  /* reorganize has its own processing */
  if (dbupdate->reorganize) {
    dbupdateReorganize (dbupdate, ffn, tagdata);
    slistFree (tagdata);
    return;
  }

  /* check-for-new, compact, rebuild, update-from-tags */

  if (dbupdate->dancefromgenre) {
    val = slistGetStr (tagdata, tagdefs [TAG_DANCE].tag);
    if (val == NULL || ! *val) {
      val = slistGetStr (tagdata, tagdefs [TAG_GENRE].tag);
      if (val != NULL && *val) {
        slistSetStr (tagdata, tagdefs [TAG_DANCE].tag, val);
      }
    }
  }

  /* a level must be set */
  val = slistGetStr (tagdata, tagdefs [TAG_DANCELEVEL].tag);
  if (val == NULL || ! *val) {
    level_t *levels;

    levels = bdjvarsdfGet (BDJVDF_LEVELS);
    slistSetStr (tagdata, tagdefs [TAG_DANCELEVEL].tag,
        levelGetDefaultName (levels));
  }

  if (logCheck (LOG_DBG, LOG_DBUPDATE)) {
    slistidx_t  iteridx;
    const char  *tag, *data;

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

  /* use the relative path here, want the dbupdate->dbtopdir stripped */
  /* before running the regex match */
  relfname = dbupdateGetRelativePath (dbupdate, ffn);

  while ((tagkey = orgIterateTagKey (dbupdate->org, &orgiteridx)) >= 0) {
    val = slistGetStr (tagdata, tagdefs [tagkey].tag);
    if (val != NULL && *val) {
      /* this tag already exists in the tagdata, keep it */
      logMsg (LOG_DBG, LOG_DBUPDATE, "  keep existing %s", tagdefs [tagkey].tag);
      continue;
    }

    /* a regex check */
    val = orgGetFromPath (dbupdate->org, relfname, tagkey);
    if (val != NULL && *val) {
      slistSetStr (tagdata, tagdefs [tagkey].tag, val);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  regex: %s : %s", tagdefs [tagkey].tag, val);
    }
  }

  songfname = ffn;
  if (dbupdate->usingmusicdir) {
    songfname = relfname;
  }

  rrn = MUSICDB_ENTRY_NEW;
  /* rebuild and compact create entirely new databases */
  /* always use a new rrn */
  if (! dbupdate->rebuild && ! dbupdate->compact) {
    song = dbGetByName (dbupdate->musicdb, songfname);
    if (song != NULL) {
      const char  *tmp;

      rrn = songGetNum (song, TAG_RRN);
      tmp = songGetStr (song, TAG_DBADDDATE);
      if (tmp != NULL) {
        slistSetStr (tagdata, tagdefs [TAG_DBADDDATE].tag, tmp);
      }
    }
  }

  /* set the prefix length. this is used by export to/import from */
  /* so that secondary folders can preserve the directory structure */
  {
    char    tmp [40];

    snprintf (tmp, sizeof (tmp), "%d", dbupdate->prefixlen);
    slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, tmp);
  }

  /* the dbWrite() procedure will set the FILE tag */
  currdb = dbupdate->musicdb;
  if (dbupdate->cleandatabase) {
    currdb = dbupdate->newmusicdb;
  }
  len = dbWrite (currdb, songfname, tagdata, rrn);
  if (rrn == MUSICDB_ENTRY_NEW && ! dbupdate->compact) {
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
  const char  *songfname;
  song_t      *song;
  slist_t     *newtaglist;

  if (ffn == NULL) {
    return;
  }

  songfname = ffn;
  if (dbupdate->usingmusicdir) {
    songfname = dbupdateGetRelativePath (dbupdate, ffn);
  }
  song = dbGetByName (dbupdate->musicdb, songfname);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  newtaglist = songTagList (song);
  audiotagWriteTags (ffn, tagdata, newtaglist, AF_REWRITE_NONE, AT_UPDATE_MOD_TIME);
  slistFree (newtaglist);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
  dbupdateIncCount (dbupdate, C_WRITE_TAGS);
}

static void
dbupdateFromiTunes (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata)
{
  const char  *songfname = NULL;
  song_t      *song = NULL;
  slist_t     *newtaglist = NULL;
  int         tagidx;
  nlistidx_t  iteridx;
  nlist_t     *entry = NULL;
  bool        changed = false;
  musicdb_t   *currdb = NULL;

  if (ffn == NULL) {
    return;
  }

  songfname = ffn;
  if (dbupdate->usingmusicdir) {
    songfname = dbupdateGetRelativePath (dbupdate, ffn);
  }
  song = dbGetByName (dbupdate->musicdb, songfname);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  currdb = dbupdate->musicdb;
  if (dbupdate->cleandatabase) {
    currdb = dbupdate->newmusicdb;
  }

  /* for itunes, just update the song data directly, */
  /* write the song to the db, and write the song tags */
  entry = itunesGetSongDataByName (dbupdate->itunes, songfname);
  changed = false;
  if (entry != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "upd-from-itunes: found %s", songfname);
    nlistStartIterator (entry, &iteridx);
    while ((tagidx = nlistIterateKey (entry, &iteridx)) >= 0) {
      if (itunesGetField (dbupdate->itunes, tagidx) <= 0) {
        /* the field is not marked to be imported */
        continue;
      }

      if (tagdefs [tagidx].valueType == VALUE_NUM) {
        long      val;

        val = nlistGetNum (entry, tagidx);
        if (songGetNum (song, tagidx) != val) {
          songSetNum (song, tagidx, val);
          logMsg (LOG_DBG, LOG_DBUPDATE, "upd-from-itunes: %s %ld", tagdefs [tagidx].tag, val);
          changed = true;
        }
      } else {
        const char  *oval;
        const char  *val;

        oval = songGetStr (song, tagidx);
        val = nlistGetStr (entry, tagidx);
        if (val != NULL &&
            (oval == NULL || strcmp (oval, val) != 0)) {
          songSetStr (song, tagidx, val);
          logMsg (LOG_DBG, LOG_DBUPDATE, "upd-from-itunes: %s %s", tagdefs [tagidx].tag, val);
          changed = true;
        }
      }
    }

    if (changed) {
      newtaglist = songTagList (song);
      dbWriteSong (currdb, song);
      audiotagWriteTags (ffn, tagdata, newtaglist, AF_REWRITE_NONE, AT_UPDATE_MOD_TIME);
      slistFree (newtaglist);
      dbupdateIncCount (dbupdate, C_UPDATED);
    }
  }
  dbupdateIncCount (dbupdate, C_FILE_PROC);
}

static void
dbupdateReorganize (dbupdate_t *dbupdate, const char *ffn, slist_t *tagdata)
{
  const char  *songfname = NULL;
  song_t      *song = NULL;
  musicdb_t   *currdb = NULL;
  char        *newfn = NULL;
  char        tbuff [MAXPATHLEN];
  char        newffn [MAXPATHLEN];
  const char  *bypass = NULL;
  int         rc;
  pathinfo_t  *pi;

  if (ffn == NULL) {
    return;
  }

  songfname = ffn;
  if (dbupdate->usingmusicdir) {
    songfname = dbupdateGetRelativePath (dbupdate, ffn);
  }

  song = dbGetByName (dbupdate->musicdb, songfname);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }
  if (songGetNum (song, TAG_DB_LOC_LOCK)) {
    /* user requested location lock */
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  currdb = dbupdate->musicdb;
  if (dbupdate->cleandatabase) {
    currdb = dbupdate->newmusicdb;
  }

  bypass = "";
  if (dbupdate->orgold != NULL) {
    bypass = orgGetFromPath (dbupdate->orgold, songGetStr (song, TAG_URI),
        (tagdefkey_t) ORG_TAG_BYPASS);
  }
  newfn = orgMakeSongPath (dbupdate->org, song, bypass);
  if (strcmp (newfn, songfname) == 0) {
    /* no change */
    dataFree (newfn);
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  audiosrcFullPath (newfn, newffn, sizeof (newffn));

  if (*newffn && fileopFileExists (newffn)) {
    /* unable to rename */
    dataFree (newfn);
    dbupdateIncCount (dbupdate, C_CANNOT_RENAME);
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  pi = pathInfo (newffn);
  snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->dlen, pi->dirname);
  if (! fileopIsDirectory (tbuff)) {
    rc = diropMakeDir (tbuff);
    if (rc != 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "unable to create dir %s", tbuff);
    }
  }

  rc = filemanipMove (ffn, newffn);
  if (rc != 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename %s", newffn);
    /* unable to rename */
    dataFree (newfn);
    dbupdateIncCount (dbupdate, C_CANNOT_RENAME);
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  songSetStr (song, TAG_URI, newfn);
  dbWriteSong (currdb, song);

  if (audiosrcOriginalExists (ffn)) {
    strlcpy (tbuff, ffn, sizeof (tbuff));
    strlcat (tbuff, bdjvarsGetStr (BDJV_ORIGINAL_EXT), sizeof (tbuff));
    strlcat (newffn, bdjvarsGetStr (BDJV_ORIGINAL_EXT), sizeof (newffn));
    rc = filemanipMove (tbuff, newffn);
    if (rc != 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "unable to rename original %s", newffn);
    }
  }

  dataFree (newfn);
  dbupdateIncCount (dbupdate, C_RENAMED);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
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

/* this gets the relative path w/o regards to whether the standard musicdir */
/* or a secondary directory is in use */
/* the full file name is always passed in */
static const char *
dbupdateGetRelativePath (dbupdate_t *dbupdate, const char *fn)
{
  const char  *p;

  p = fn;
  p += dbupdate->dbtopdirlen + 1;
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
  olddirs = mdstrdup (dbupdate->olddirlist);
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
  mdfree (olddirs);
  return false;
}

static void
dbupdateIncCount (dbupdate_t *dbupdate, int tag)
{
  ++dbupdate->counts [tag];
}

