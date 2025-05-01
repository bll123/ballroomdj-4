/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
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
 *      no new database is created.
 *    - compact
 *      create a new db file, bypass any deleted entries.
 *      may be used in conjunction w/check-for-new.
 *      a new database is created.
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
#include "dance.h"
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
#include "pathinfo.h"
#include "progstate.h"
#include "procutil.h"
#include "queue.h"
#include "rafile.h"
#include "slist.h"
#include "sockh.h"
#include "song.h"
#include "songdb.h"
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
  C_AUDIO_TAGS_PARSED,
  C_FILE_COUNT,
  C_FILE_PROC,
  C_FILE_QUEUED,
  C_FILE_SKIPPED,
  C_IN_DB,
  C_NEW,
  C_QUEUE_MAX,
  C_RENAMED,
  C_RENAME_FAIL,
  C_SKIP_BAD,
  C_SKIP_BDJ_OLD_DIR,
  C_SKIP_DEL,
  C_SKIP_NON_AUDIO,
  C_SKIP_NO_TAGS,
  C_SKIP_ORIG,
  C_UPDATED,
  C_WRITE_TAGS,
  C_MAX,
};

typedef struct {
  char        *ffn;
  char        *songfn;
  char        *relfn;
} tagdataitem_t;

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  uint32_t          startflags;
  int               state;
  musicdb_t         *musicdb;
  musicdb_t         *newmusicdb;
  songdb_t          *songdb;
  const char        *musicdir;
  size_t            musicdirlen;
  const char        *processmusicdir;
  int               prefixlen;
  size_t            processmusicdirlen;
  mstime_t          outputTimer;
  org_t             *org;
  org_t             *orgold;
  asiter_t          *asiter;
  slistidx_t        dbiter;
  bdjregex_t        *badfnregex;
  dbidx_t           counts [C_MAX];
  mstime_t          starttm;
  int               stopwaitcount;
  const char        *olddirlist;
  itunes_t          *itunes;
  queue_t           *tagdataq;
  /* base database operations */
  bool              checknew;
  bool              compact;
  bool              rebuild;
  bool              reorganize;
  bool              updfromitunes;
  bool              updfromtags;
  bool              writetags;
  /* database handling */
  bool              cleandatabase;
  /* other stuff */
  bool              autoorg;
  bool              cli;
  bool              dancefromgenre;
  bool              haveolddirlist;
  bool              iterfromaudiosrc;
  bool              iterfromdb;
  bool              progress;
  bool              stoprequest;
  bool              usingmusicdir;
  bool              verbose;
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
static void     dbupdateQueueFile (dbupdate_t *dbupdate, const char *ffn, const char *tsongfn, const char *relfn);
static void     dbupdateTagDataFree (void *data);
static void     dbupdateProcessFileQueue (dbupdate_t *dbupdate);
static void     dbupdateProcessFile (dbupdate_t *dbupdate, tagdataitem_t *tdi);
static void     dbupdateWriteTags (dbupdate_t *dbupdate, tagdataitem_t *tdi, slist_t *tagdata);
static void     dbupdateFromiTunes (dbupdate_t *dbupdate, tagdataitem_t *tdi);
static void     dbupdateReorganize (dbupdate_t *dbupdate, tagdataitem_t *tdi, int songdbdefault);
static void     dbupdateSigHandler (int sig);
static void     dbupdateOutputProgress (dbupdate_t *dbupdate);
static bool     checkOldDirList (dbupdate_t *dbupdate, const char *fn);
static void     dbupdateIncCount (dbupdate_t *dbupdate, int tag);
static void     dbupdateWriteSong (dbupdate_t *dbupdate, song_t *song, int32_t *songdbflags, dbidx_t rrn);
static musicdb_t * dbupdateSetCurrentDB (dbupdate_t *dbupdate);
static const char * dbupdateIterate (dbupdate_t *dbupdate);

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
  dbupdate.dbiter = -1;
  for (int i = 0; i < C_MAX; ++i) {
    dbupdate.counts [i] = 0;
  }
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
  dbupdate.haveolddirlist = false;
  dbupdate.iterfromaudiosrc = false;
  dbupdate.iterfromdb = false;
  dbupdate.olddirlist = NULL;
  dbupdate.progress = false;
  dbupdate.stoprequest = false;
  dbupdate.usingmusicdir = true;
  dbupdate.verbose = false;
  mstimeset (&dbupdate.outputTimer, 0);
  dbupdate.autoorg = false;
  dbupdate.dancefromgenre = false;

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
  logProcBegin ();

  dbupdate.autoorg = bdjoptGetNum (OPT_G_AUTOORGANIZE);
  dbupdate.dancefromgenre = bdjoptGetNum (OPT_G_LOADDANCEFROMGENRE);

  dbupdate.songdb = songdbAlloc (dbupdate.musicdb);

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

  if ((dbupdate.startflags & BDJ4_ARG_DB_CHECK_NEW) == BDJ4_ARG_DB_CHECK_NEW) {
    dbupdate.checknew = true;
    dbupdate.iterfromaudiosrc = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== check-new");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_COMPACT) == BDJ4_ARG_DB_COMPACT) {
    dbupdate.compact = true;
    dbupdate.iterfromdb = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== compact");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_REBUILD) == BDJ4_ARG_DB_REBUILD) {
    dbupdate.rebuild = true;
    dbupdate.iterfromaudiosrc = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== rebuild");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_UPD_FROM_TAGS) == BDJ4_ARG_DB_UPD_FROM_TAGS) {
    dbupdate.updfromtags = true;
    dbupdate.iterfromdb = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== upd-from-tags");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_UPD_FROM_ITUNES) == BDJ4_ARG_DB_UPD_FROM_ITUNES) {
    dbupdate.updfromitunes = true;
    dbupdate.itunes = itunesAlloc ();
    dbupdate.iterfromdb = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== upd-from-itunes");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_WRITE_TAGS) == BDJ4_ARG_DB_WRITE_TAGS) {
    dbupdate.writetags = true;
    dbupdate.iterfromdb = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== write-tags");
  }
  if ((dbupdate.startflags & BDJ4_ARG_DB_REORG) == BDJ4_ARG_DB_REORG) {
    dbupdate.reorganize = true;
    dbupdate.iterfromdb = true;
    logMsg (LOG_DBG, LOG_IMPORTANT, "== reorganize");
  }
  if ((dbupdate.startflags & BDJ4_ARG_PROGRESS) == BDJ4_ARG_PROGRESS) {
    dbupdate.progress = true;
  }
  if ((dbupdate.startflags & BDJ4_ARG_CLI) == BDJ4_ARG_CLI) {
    dbupdate.cli = true;
#if _define_SIGCHLD
    osDefaultSignal (SIGCHLD);
#endif
  }
  if ((dbupdate.startflags & BDJ4_ARG_VERBOSE) == BDJ4_ARG_VERBOSE) {
    dbupdate.verbose = true;
  }

  dbupdate.conn = connInit (ROUTE_DBUPDATE);

  listenPort = bdjvarsGetNum (BDJVL_PORT_DBUPDATE);
  sockhMainLoop (listenPort, dbupdateProcessMsg, dbupdateProcessing, &dbupdate);
  connFree (dbupdate.conn);
  progstateFree (dbupdate.progstate);

  logProcEnd ("");
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
  int         stop = SOCKH_CONTINUE;
  char        *msg = NULL;


  if (! progstateIsRunning (dbupdate->progstate)) {
    progstateProcess (dbupdate->progstate);
    if (progstateCurrState (dbupdate->progstate) == STATE_CLOSED) {
      stop = SOCKH_STOP;
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
    char  tbuff [MAXPATHLEN];

    if (dbupdate->rebuild) {
      dbupdate->cleandatabase = true;
    }
    if (dbupdate->compact) {
      dbupdate->cleandatabase = true;
    }

    dbBackup ();

    if (dbupdate->cleandatabase) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "new database");
      pathbldMakePath (tbuff, sizeof (tbuff),
          MUSICDB_TMP_FNAME, MUSICDB_EXT, PATHBLD_MP_DREL_DATA);
      fileopDelete (tbuff);
      dbupdate->newmusicdb = dbOpen (tbuff);
      dbStartBatch (dbupdate->newmusicdb);
    }

    logMsg (LOG_DBG, LOG_BASIC, "existing db count: %" PRId32, dbCount (dbupdate->musicdb));
    dbStartBatch (dbupdate->musicdb);

    dbupdate->state = DB_UPD_PREP;
  }

  if (dbupdate->state == DB_UPD_PREP) {
    char  tmp [40];
    char  tbuff [200];
    char  *tstr;

    mstimestart (&dbupdate->starttm);

    dbupdateOutputProgress (dbupdate);

    dbupdate->prefixlen = 0;
    dbupdate->usingmusicdir = true;
    dbupdate->musicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    dbupdate->musicdirlen = strlen (dbupdate->musicdir);

    /* the default */
    dbupdate->processmusicdir = bdjoptGetStr (OPT_M_DIR_MUSIC);
    /* check if a different music-dir was specified */
    tstr = bdjvarsGetStr (BDJV_MUSIC_DIR);
    if (tstr != NULL && strcmp (tstr, dbupdate->musicdir) != 0) {
      dbupdate->usingmusicdir = false;
      dbupdate->processmusicdir = tstr;
      dbupdate->prefixlen = strlen (tstr) + 1;  // include the slash
    }
    dbupdate->processmusicdirlen = strlen (dbupdate->processmusicdir) + 1;

    if (dbupdate->iterfromaudiosrc) {
      logMsg (LOG_DBG, LOG_BASIC, "processmusicdir %s", dbupdate->processmusicdir);

      dbupdate->asiter = audiosrcStartIterator (AUDIOSRC_TYPE_FILE,
          AS_ITER_DIR, dbupdate->processmusicdir, NULL, -1);

      if (dbupdate->asiter != NULL) {
        dbupdate->counts [C_FILE_COUNT] = audiosrcIterCount (dbupdate->asiter);
        logMsg (LOG_DBG, LOG_IMPORTANT, "read directory %s: %" PRId64 " ms",
            dbupdate->processmusicdir, (int64_t) mstimeend (&dbupdate->starttm));
        logMsg (LOG_DBG, LOG_IMPORTANT, "  %" PRId32 " files found", dbupdate->counts [C_FILE_COUNT]);
      }
    }

    if (dbupdate->iterfromdb) {
      dbStartIterator (dbupdate->musicdb, &dbupdate->dbiter);
      dbupdate->counts [C_FILE_COUNT] = dbCount (dbupdate->musicdb);
      logMsg (LOG_DBG, LOG_IMPORTANT, "prep: %" PRId64 " ms",
          (int64_t) mstimeend (&dbupdate->starttm));
      logMsg (LOG_DBG, LOG_IMPORTANT, "  %" PRId32 " files found", dbupdate->counts [C_FILE_COUNT]);
    }

    /* message to manageui */
    snprintf (tmp, sizeof (tmp), "%" PRId32, dbupdate->counts [C_FILE_COUNT]);
    /* CONTEXT: database update: status message */
    snprintf (tbuff, sizeof (tbuff), _("%s files found"), tmp);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    dbupdate->state = DB_UPD_PROC_FN;
  }

  /* need to check the queue count so that too much memory is not used */
  /* all at once */
  if (dbupdate->state == DB_UPD_PROC_FN &&
      queueGetCount (dbupdate->tagdataq) <= QUEUE_PROCESS_LIMIT) {
    const char  *fn;
    pathinfo_t  *pi;
    song_t      *prevsong = NULL;

    while ((fn = dbupdateIterate (dbupdate)) != NULL) {
      song_t      *song;
      char        ffn [MAXPATHLEN];
      const char  *tsongfn;     // the db entry filename
      const char  *relfn;       // the relative path name

      /* if the filename is from the audio-source iterator, */
      /*    it is a full path */
      /* if the filename is from the db iterator, */
      /*    it could be a full path or a relative path */
      tsongfn = fn;
      stpecpy (ffn, ffn + sizeof (ffn), fn);
      if (dbupdate->iterfromaudiosrc) {
        tsongfn = audiosrcRelativePath (fn, 0);
      }
      if (dbupdate->iterfromdb) {
        audiosrcFullPath (fn, ffn, sizeof (ffn), NULL, 0);
      }
      song = dbGetByName (dbupdate->musicdb, tsongfn);
      relfn = tsongfn;
      if (dbupdate->prefixlen > 0) {
        relfn = ffn + dbupdate->prefixlen;
      }
      // fprintf (stderr, "--    fn: %s\n", fn);        //
      // fprintf (stderr, "     ffn: %s\n", ffn);       //
      // fprintf (stderr, " tsongfn: %s\n", tsongfn);   //
      // fprintf (stderr, " relfn-a: %s\n", relfn);     //
      // fprintf (stderr, "    type: %d\n", audiosrcGetType (fn));

      if (dbupdate->iterfromaudiosrc) {
        pi = pathInfo (fn);
        /* fast skip of some known file extensions that might show up */
        if (pathInfoExtCheck (pi, ".jpg") ||
            pathInfoExtCheck (pi, ".png") ||
            pathInfoExtCheck (pi, ".bak") ||
            pathInfoExtCheck (pi, ".txt") ||
            pathInfoExtCheck (pi, ".svg")) {
          dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
          dbupdateIncCount (dbupdate, C_SKIP_NON_AUDIO);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-not-audio");
          pathInfoFree (pi);
          continue;
        }
        if (pathInfoExtCheck (pi, BDJ4_GENERIC_ORIG_EXT) ||
            pathInfoExtCheck (pi, bdjvarsGetStr (BDJV_ORIGINAL_EXT))) {
          dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
          dbupdateIncCount (dbupdate, C_SKIP_ORIG);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-orig");
          pathInfoFree (pi);
          continue;
        }
        if (strncmp (pi->filename, bdjvarsGetStr (BDJV_DELETE_PFX),
            bdjvarsGetNum (BDJVL_DELETE_PFX_LEN)) == 0) {
          dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
          dbupdateIncCount (dbupdate, C_SKIP_DEL);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-del");
          pathInfoFree (pi);
          continue;
        }
        pathInfoFree (pi);

        if (dbupdate->haveolddirlist &&
            checkOldDirList (dbupdate, fn)) {
          dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
          dbupdateIncCount (dbupdate, C_SKIP_BDJ_OLD_DIR);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  skip-old-dir");
          continue;
        }
      }

      /* check to see if the audio file is already in the database */
      /* this is done for all modes except for rebuild */
      /* 'checknew' skips any processing for an audio file */
      /* that is already present unless the compact flag is on */
      if (! dbupdate->rebuild && fn != NULL) {
        int         pfxlen;

        if (song != NULL) {
          pfxlen = songGetNum (song, TAG_PREFIX_LEN);
          if (pfxlen > 0) {
            relfn = fn + pfxlen;
            // fprintf (stderr, "relfn-b: %s\n", relfn);     //
          }

          if (dbupdate->compact && prevsong != NULL) {
            const char  *puri;
            const char  *curi;

            curi = songGetStr (song, TAG_URI);
            puri = songGetStr (prevsong, TAG_URI);
            if (strcmp (curi, puri) == 0) {
              /* skip any duplicates */
              dbupdateOutputProgress (dbupdate);
              prevsong = song;
              continue;
            }
          }

          dbupdateIncCount (dbupdate, C_IN_DB);
          logMsg (LOG_DBG, LOG_DBUPDATE, "  in-database (%" PRId32 ") ", dbupdate->counts [C_IN_DB]);

          /* if doing a checknew, no need for further processing */
          /* if doing a compact, the information must be written to */
          /* the new database. */
          if (dbupdate->checknew || dbupdate->compact) {
            if (dbupdate->checknew && ! dbupdate->compact) {
              dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
            }
            if (dbupdate->compact) {
              dbupdateIncCount (dbupdate, C_FILE_PROC);
              dbupdateIncCount (dbupdate, C_UPDATED);
              /* the audio files are not being modified, */
              /* using dbWriteSong() here is ok */
              songSetNum (song, TAG_RRN, MUSICDB_ENTRY_NEW);
              dbWriteSong (dbupdate->newmusicdb, song);
            }

            dbupdateOutputProgress (dbupdate);
            if (queueGetCount (dbupdate->tagdataq) >= QUEUE_PROCESS_LIMIT) {
              break;
            }

            /* compact needs the previous song to check for dups */
            prevsong = song;
            continue;
          }
        }
      }

      if (regexMatch (dbupdate->badfnregex, fn)) {
        dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
        dbupdateIncCount (dbupdate, C_SKIP_BAD);
        logMsg (LOG_DBG, LOG_DBUPDATE, "  bad fn-regex (%" PRId32 ") ", dbupdate->counts [C_SKIP_BAD]);
        continue;
      }

      dbupdateQueueFile (dbupdate, ffn, tsongfn, relfn);

      dbupdateIncCount (dbupdate, C_FILE_QUEUED);
      if (queueGetCount (dbupdate->tagdataq) >= QUEUE_PROCESS_LIMIT) {
        break;
      }
    }

    if (fn == NULL) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- skipped (%" PRId32 ")", dbupdate->counts [C_FILE_SKIPPED]);
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- all filenames sent (%" PRId32 "): %" PRId64 " ms",
          dbupdate->counts [C_FILE_QUEUED], (int64_t) mstimeend (&dbupdate->starttm));
      dbupdate->state = DB_UPD_PROCESS;
    }
  }

  if (dbupdate->state == DB_UPD_PROC_FN ||
      dbupdate->state == DB_UPD_PROCESS) {
    dbupdateProcessFileQueue (dbupdate);
    dbupdateOutputProgress (dbupdate);

    logMsg (LOG_DBG, LOG_DBUPDATE, "progress: %" PRId32 "+%" PRId32 "(%" PRId32 ") >= %" PRId32 "",
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
              "found %" PRId32 " skip %" PRId32 " indb %" PRId32 " new %" PRId32 " updated %" PRId32 " renamed %" PRId32 " norename %" PRId32 " notaudio %" PRId32 " writetag %" PRId32 "\n",
              dbupdate->counts [C_FILE_COUNT],
              dbupdate->counts [C_FILE_SKIPPED],
              dbupdate->counts [C_IN_DB],
              dbupdate->counts [C_NEW],
              dbupdate->counts [C_UPDATED],
              dbupdate->counts [C_RENAMED],
              dbupdate->counts [C_RENAME_FAIL],
              dbupdate->counts [C_SKIP_NON_AUDIO],
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
    snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Total Files"), dbupdate->counts [C_FILE_COUNT]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    if (! dbupdate->rebuild) {
      /* CONTEXT: database update: status message: files found in the database */
      snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Loaded from Database"), dbupdate->counts [C_IN_DB]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }

    /* CONTEXT: database update: status message: new files saved to the database */
    snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("New Files"), dbupdate->counts [C_NEW]);
    connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);

    if (! dbupdate->rebuild && ! dbupdate->writetags) {
      /* CONTEXT: database update: status message: number of files updated in the database */
      snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Updated"), dbupdate->counts [C_UPDATED]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }

    if (dbupdate->reorganize || dbupdate->checknew || dbupdate->rebuild) {
      if (dbupdate->counts [C_RENAMED] > 0) {
        /* CONTEXT: database update: status message: number of files renamed */
        snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Renamed"), dbupdate->counts [C_RENAMED]);
        connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
      }
      if (dbupdate->counts [C_RENAME_FAIL] > 0) {
        /* CONTEXT: database update: status message: number of files that cannot be renamed */
        snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Cannot Rename"), dbupdate->counts [C_RENAME_FAIL]);
        connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
      }
    }

    if (dbupdate->writetags) {
      /* re-use the 'Updated' label for write-tags */
      /* CONTEXT: database update: status message: number of files updated */
      snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Updated"), dbupdate->counts [C_WRITE_TAGS]);
      connSendMessage (dbupdate->conn, ROUTE_MANAGEUI, MSG_DB_STATUS_MSG, tbuff);
    }

    /* CONTEXT: database update: status message: other files that cannot be processed */
    snprintf (tbuff, sizeof (tbuff), "%s : %" PRId32 "", _("Other Files"),
        dbupdate->counts [C_SKIP_BAD] +
        dbupdate->counts [C_SKIP_NO_TAGS] + dbupdate->counts [C_SKIP_NON_AUDIO] +
        dbupdate->counts [C_SKIP_ORIG] +
        dbupdate->counts [C_SKIP_BDJ_OLD_DIR] + dbupdate->counts [C_SKIP_DEL]);
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
    logMsg (LOG_DBG, LOG_IMPORTANT, "    found: %" PRId32 "", dbupdate->counts [C_FILE_COUNT]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  %" PRId32 "", dbupdate->counts [C_FILE_SKIPPED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "   queued: %" PRId32 "", dbupdate->counts [C_FILE_QUEUED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "processed: %" PRId32 "", dbupdate->counts [C_FILE_PROC]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "queue-max: %" PRId32 "", dbupdate->counts [C_QUEUE_MAX]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "    in-db: %" PRId32 "", dbupdate->counts [C_IN_DB]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "audio-tag: %" PRId32 "", dbupdate->counts [C_AUDIO_TAGS_PARSED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      new: %" PRId32 "", dbupdate->counts [C_NEW]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  updated: %" PRId32 "", dbupdate->counts [C_UPDATED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  renamed: %" PRId32 "", dbupdate->counts [C_RENAMED]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "no rename: %" PRId32 "", dbupdate->counts [C_RENAME_FAIL]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "write-tag: %" PRId32 "", dbupdate->counts [C_WRITE_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "      bad: %" PRId32 "", dbupdate->counts [C_SKIP_BAD]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  no tags: %" PRId32 "", dbupdate->counts [C_SKIP_NO_TAGS]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "not-audio: %" PRId32 "", dbupdate->counts [C_SKIP_NON_AUDIO]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "orig-skip: %" PRId32 "", dbupdate->counts [C_SKIP_ORIG]);
    logMsg (LOG_DBG, LOG_IMPORTANT, " del-skip: %" PRId32 "", dbupdate->counts [C_SKIP_DEL]);
    logMsg (LOG_DBG, LOG_IMPORTANT, "  old-dir: %" PRId32 "", dbupdate->counts [C_SKIP_BDJ_OLD_DIR]);

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

  logProcBegin ();

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

  logProcEnd ("");
  return rc;
}

static bool
dbupdateHandshakeCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;
  bool          rc = STATE_NOT_FINISH;

  logProcBegin ();

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

  logProcEnd ("");
  return rc;
}

static bool
dbupdateStoppingCallback (void *tdbupdate, programstate_t programState)
{
  dbupdate_t    *dbupdate = tdbupdate;

  logProcBegin ();

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, PROCUTIL_NORM_TERM);
  connDisconnect (dbupdate->conn, ROUTE_MANAGEUI);
  logProcEnd ("");
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

  logProcBegin ();

  audiosrcCleanIterator (dbupdate->asiter);

  bdj4shutdown (ROUTE_DBUPDATE, dbupdate->musicdb);
  dbClose (dbupdate->newmusicdb);

  procutilStopAllProcess (dbupdate->processes, dbupdate->conn, PROCUTIL_FORCE_TERM);
  procutilFreeAll (dbupdate->processes);

  songdbFree (dbupdate->songdb);
  itunesFree (dbupdate->itunes);
  orgFree (dbupdate->org);
  orgFree (dbupdate->orgold);
  regexFree (dbupdate->badfnregex);
  queueFree (dbupdate->tagdataq);

  logProcEnd ("");
  return STATE_FINISHED;
}

static void
dbupdateQueueFile (dbupdate_t *dbupdate, const char *ffn,
    const char *songfn, const char *relfn)
{
  tagdataitem_t *tdi;
  dbidx_t       count;

  if (dbupdate->stoprequest) {
    /* crashes below (strdup (ffn)) if the requests are processed after */
    /* receiving a stop-request */
    /* do not know why exactly */
    return;
  }

  tdi = mdmalloc (sizeof (tagdataitem_t));
  tdi->ffn = mdstrdup (ffn);
  tdi->songfn = mdstrdup (songfn);
  tdi->relfn = mdstrdup (relfn);
  queuePush (dbupdate->tagdataq, tdi);
  count = queueGetCount (dbupdate->tagdataq);
  // fprintf (stderr, "q-push: %s %" PRId32 "\n", ffn, count);
  if (count > dbupdate->counts [C_QUEUE_MAX]) {
    dbupdate->counts [C_QUEUE_MAX] = count;
  }
}

static void
dbupdateTagDataFree (void *data)
{
  tagdataitem_t   *tdi = data;

  if (tdi != NULL) {
    dataFree (tdi->ffn);
    dataFree (tdi->songfn);
    dataFree (tdi->relfn);
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
  dbupdateProcessFile (dbupdate, tdi);
  dbupdateTagDataFree (tdi);
}

static void
dbupdateProcessFile (dbupdate_t *dbupdate, tagdataitem_t *tdi)
{
  slist_t     *tagdata = NULL;
  slistidx_t  orgiteridx;
  int         tagkey;
  dbidx_t     rrn;
  song_t      *song = NULL;
  const char  *val = NULL;
  int         rewrite;
  int32_t     songdbflags;

  logMsg (LOG_DBG, LOG_DBUPDATE, "__ process %s", tdi->ffn);

  if (! dbupdate->updfromitunes &&
      ! dbupdate->reorganize &&
      ! dbupdate->compact) {
    /* write-tags needs the tag-data to determine updates */
    /* new audio files need the tag-data */
    /* compact gets the tag-data from the database */

    tagdata = audiotagParseData (tdi->ffn, &rewrite);
    dbupdateIncCount (dbupdate, C_AUDIO_TAGS_PARSED);
    if (slistGetCount (tagdata) == 0) {
      /* if there is not even a duration, then file is no good */
      /* probably not an audio file */
      logMsg (LOG_DBG, LOG_DBUPDATE, "  no tags");
      dbupdateIncCount (dbupdate, C_SKIP_NO_TAGS);
      dbupdateIncCount (dbupdate, C_FILE_PROC);
      return;
    }
  }

  /* write-tags has its own processing */
  if (dbupdate->writetags) {
    dbupdateWriteTags (dbupdate, tdi, tagdata);
    slistFree (tagdata);
    return;
  }

  /* update-from-itunes has its own processing */
  if (dbupdate->updfromitunes) {
    dbupdateFromiTunes (dbupdate, tdi);
    return;
  }

  /* reorganize has its own processing */
  if (dbupdate->reorganize) {
    dbupdateReorganize (dbupdate, tdi, SONGDB_FORCE_RENAME);
    return;
  }

  /* check-for-new, compact, rebuild, update-from-tags */

  if (dbupdate->dancefromgenre) {
    val = slistGetStr (tagdata, tagdefs [TAG_DANCE].tag);

    /* only try the genre->dance if the dance is not already set */
    if (val == NULL || ! *val) {
      val = slistGetStr (tagdata, tagdefs [TAG_GENRE].tag);

      /* if a genre is present */
      if (val != NULL && *val) {
        datafileconv_t  conv;

        /* only set the dance from the genre if the genre->dance */
        /* conversion is valid */
        conv.invt = VALUE_STR;
        conv.str = val;
        danceConvDance (&conv);
        if (conv.num >= 0) {
          slistSetStr (tagdata, tagdefs [TAG_DANCE].tag, val);
        }
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

  while ((tagkey = orgIterateTagKey (dbupdate->org, &orgiteridx)) >= 0) {
    val = slistGetStr (tagdata, tagdefs [tagkey].tag);
    if (val != NULL && *val) {
      /* this tag already exists in the tagdata, keep it */
      logMsg (LOG_DBG, LOG_DBUPDATE, "  keep existing %s", tagdefs [tagkey].tag);
      continue;
    }

    /* a regex check */
    val = orgGetFromPath (dbupdate->org, tdi->relfn, tagkey);
    if (val != NULL && *val) {
      slistSetStr (tagdata, tagdefs [tagkey].tag, val);
      logMsg (LOG_DBG, LOG_DBUPDATE, "  regex: %s : %s", tagdefs [tagkey].tag, val);
    }
  }

  /* rebuild and compact create an entirely new databases */
  /* always use a new rrn when adding a new song */

  rrn = MUSICDB_ENTRY_NEW;

  /* set the prefix length. this is used by export to/import from */
  /* so that secondary folders can preserve the directory structure */
  /* prefix length should always be set. */
  /* do this before the existing prefix length is recovered from the song */
  {
    char    tmp [40];

    snprintf (tmp, sizeof (tmp), "%d", dbupdate->prefixlen);
    slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, tmp);
  }

  /* on a rebuild, the database add date will be reset */
  /* on a compact, the rrn is not wanted, but the add date is wanted */
  /* for all other modes, the rrn, the add date and prefix-len are kept */
  if (! dbupdate->rebuild) {
    song = dbGetByName (dbupdate->musicdb, tdi->songfn);
    if (song != NULL) {
      char        tbuff [40];
      ssize_t     val;

      if (dbupdate->compact) {
        /* compact can get the tagdata from the song, there are no changes */
        tagdata = songTagList (song);
      }
      if (! dbupdate->compact) {
        rrn = songGetNum (song, TAG_RRN);
      }
      val = songGetNum (song, TAG_DBADDDATE);
      if (val > 0) {
        snprintf (tbuff, sizeof (tbuff), "%" PRIu64, (uint64_t) val);
        slistSetStr (tagdata, tagdefs [TAG_DBADDDATE].tag, tbuff);
      }
      val = songGetNum (song, TAG_PREFIX_LEN);
      if (val > 0) {
        snprintf (tbuff, sizeof (tbuff), "%" PRId64, (int64_t) val);
        slistSetStr (tagdata, tagdefs [TAG_PREFIX_LEN].tag, tbuff);
      }
      val = songGetNum (song, TAG_DB_LOC_LOCK);
      snprintf (tbuff, sizeof (tbuff), "%" PRId64, (int64_t) val);
      slistSetStr (tagdata, tagdefs [TAG_DB_LOC_LOCK].tag, tbuff);
    }
  }

  dbupdateSetCurrentDB (dbupdate);

  song = songAlloc ();
  songFromTagList (song, tagdata);
  songSetStr (song, TAG_URI, tdi->songfn);
  songdbflags = SONGDB_NONE;
  if (dbupdate->autoorg && (dbupdate->checknew || dbupdate->rebuild)) {
    songdbflags = SONGDB_NONE;
  }
  dbupdateWriteSong (dbupdate, song, &songdbflags, rrn);
  songFree (song);

  if ((songdbflags & SONGDB_RET_SUCCESS) == SONGDB_RET_SUCCESS) {
    if (rrn == MUSICDB_ENTRY_NEW && ! dbupdate->compact) {
      dbupdateIncCount (dbupdate, C_NEW);
    } else {
      dbupdateIncCount (dbupdate, C_UPDATED);
    }
  }

  slistFree (tagdata);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
}

static void
dbupdateWriteTags (dbupdate_t *dbupdate, tagdataitem_t *tdi, slist_t *tagdata)
{
  song_t      *song;
  slist_t     *newtaglist;

  if (tdi->ffn == NULL) {
    return;
  }

  song = dbGetByName (dbupdate->musicdb, tdi->songfn);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  newtaglist = songTagList (song);
  audiotagWriteTags (tdi->ffn, tagdata, newtaglist, AF_REWRITE_NONE, AT_FLAGS_MOD_TIME_UPDATE);
  slistFree (newtaglist);
  dbupdateIncCount (dbupdate, C_FILE_PROC);
  dbupdateIncCount (dbupdate, C_WRITE_TAGS);
}

static void
dbupdateFromiTunes (dbupdate_t *dbupdate, tagdataitem_t *tdi)
{
  song_t      *song = NULL;
  slist_t     *newtaglist = NULL;
  int         tagidx;
  nlistidx_t  iteridx;
  nlist_t     *entry = NULL;
  bool        changed = false;

  if (tdi->ffn == NULL) {
    return;
  }

  song = dbGetByName (dbupdate->musicdb, tdi->songfn);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  dbupdateSetCurrentDB (dbupdate);

  /* for itunes, just update the song data directly, */
  /* write the song to the db, and write the song tags */
  entry = itunesGetSongDataByName (dbupdate->itunes, tdi->songfn);
  changed = false;
  if (entry != NULL) {
    logMsg (LOG_DBG, LOG_DBUPDATE, "upd-from-itunes: found %s", tdi->songfn);
    nlistStartIterator (entry, &iteridx);
    while ((tagidx = nlistIterateKey (entry, &iteridx)) >= 0) {
      if (itunesGetField (dbupdate->itunes, tagidx) <= 0) {
        /* the field is not marked to be imported */
        continue;
      }

      if (tagdefs [tagidx].valueType == VALUE_NUM) {
        listnum_t   val;

        val = nlistGetNum (entry, tagidx);
        if (songGetNum (song, tagidx) != val) {
          songSetNum (song, tagidx, val);
          logMsg (LOG_DBG, LOG_DBUPDATE, "upd-from-itunes: %s %" PRId64, tagdefs [tagidx].tag, val);
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
      int     songdbflags = SONGDB_NONE;

      newtaglist = songTagList (song);
      dbupdateWriteSong (dbupdate, song, &songdbflags, songGetNum (song, TAG_RRN));
      slistFree (newtaglist);
      dbupdateIncCount (dbupdate, C_UPDATED);
    }
  }
  dbupdateIncCount (dbupdate, C_FILE_PROC);
}

static void
dbupdateReorganize (dbupdate_t *dbupdate, tagdataitem_t *tdi,
    int songdbdefault)
{
  song_t      *song = NULL;
  int32_t     songdbflags = songdbdefault;

  if (tdi->ffn == NULL) {
    return;
  }

  song = dbGetByName (dbupdate->musicdb, tdi->songfn);
  if (song == NULL) {
    dbupdateIncCount (dbupdate, C_FILE_PROC);
    return;
  }

  dbupdateSetCurrentDB (dbupdate);
  dbupdateWriteSong (dbupdate, song, &songdbflags, songGetNum (song, TAG_RRN));
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

static bool
checkOldDirList (dbupdate_t *dbupdate, const char *fn)
{
  const char  *fp;
  char        *olddirs;
  char        *p;
  char        *tokstr;
  size_t      len;

  fp = fn + dbupdate->processmusicdirlen;
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

static void
dbupdateWriteSong (dbupdate_t *dbupdate, song_t *song,
    int32_t *songdbflags, dbidx_t rrn)
{
  songSetChanged (song);
  songdbWriteDBSong (dbupdate->songdb, song, songdbflags, rrn);

  if ((*songdbflags & SONGDB_RET_NULL) == SONGDB_RET_NULL) {
    dbupdateIncCount (dbupdate, C_SKIP_BAD);
    dbupdateIncCount (dbupdate, C_FILE_SKIPPED);
    logMsg (LOG_DBG, LOG_DBUPDATE, "  bad-null (%" PRId32 ") ", dbupdate->counts [C_SKIP_BAD]);
  } else if ((*songdbflags & SONGDB_RET_RENAME_SUCCESS) == SONGDB_RET_RENAME_SUCCESS) {
    dbupdateIncCount (dbupdate, C_RENAMED);
  } else if ((*songdbflags & SONGDB_RET_REN_FILE_EXISTS) == SONGDB_RET_REN_FILE_EXISTS) {
    dbupdateIncCount (dbupdate, C_RENAME_FAIL);
  } else if ((*songdbflags & SONGDB_RET_RENAME_FAIL) == SONGDB_RET_RENAME_FAIL) {
    dbupdateIncCount (dbupdate, C_RENAME_FAIL);
  }
}

static musicdb_t *
dbupdateSetCurrentDB (dbupdate_t *dbupdate)
{
  musicdb_t   *currdb;

  currdb = dbupdate->musicdb;
  if (dbupdate->cleandatabase) {
    currdb = dbupdate->newmusicdb;
  }
  songdbSetMusicDB (dbupdate->songdb, currdb);
  return currdb;
}

static const char *
dbupdateIterate (dbupdate_t *dbupdate)
{
  const char  *fn = NULL;
  dbidx_t     dbidx;
  song_t      *song;

  if (dbupdate->iterfromaudiosrc) {
    fn = audiosrcIterate (dbupdate->asiter);
  }
  if (dbupdate->iterfromdb) {
    song = dbIterate (dbupdate->musicdb, &dbidx, &dbupdate->dbiter);
    fn = songGetStr (song, TAG_URI);
  }

  return fn;
}
