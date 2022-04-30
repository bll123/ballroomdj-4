/*
 *  bdj4dbtag
 *  reads the tags from an audio file.
 *  uses pthreads.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>

#include "audiotag.h"
#include "bdj4.h"
#include "bdj4init.h"
#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "bdjvarsdf.h"
#include "bdjvars.h"
#include "conn.h"
#include "lock.h"
#include "log.h"
#include "osutils.h"
#include "pathbld.h"
#include "progstate.h"
#include "procutil.h"
#include "queue.h"
#include "slist.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"

#define DBTAG_TMP_FN        "dbtag"

enum {
  DBTAG_T_STATE_INIT,
  DBTAG_T_STATE_ACTIVE,
  DBTAG_T_STATE_HAVE_DATA,
};

typedef struct {
  pthread_t   thread;
  int         state;
  int         idx;
  long        count;
  char        *fn;
  char        * data;
} dbthread_t;

typedef struct {
  progstate_t       *progstate;
  procutil_t        *processes [ROUTE_MAX];
  conn_t            *conn;
  char              *locknm;
  int               numActiveThreads;
  dbidx_t           received;
  dbidx_t           sent;
  dbidx_t           maxqueuelen;
  queue_t           *fileQueue;
  int               maxThreads;
  dbthread_t        *threads;
  mstime_t          starttm;
  int               iterations;
  int               threadActiveSum;
  bool              running : 1;
} dbtag_t;

static int      dbtagProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
                    bdjmsgmsg_t msg, char *args, void *udata);
static int      dbtagProcessing (void *udata);
static bool     dbtagConnectingCallback (void *tdbtag, programstate_t programState);
static bool     dbtagHandshakeCallback (void *tdbtag, programstate_t programState);
static bool     dbtagStoppingCallback (void *tdbtag, programstate_t programState);
static bool     dbtagClosingCallback (void *tdbtag, programstate_t programState);
static void     dbtagProcessFileMsg (dbtag_t *dbtag, char *args);
static void     * dbtagProcessFile (void *tdbthread);
static void     dbtagSigHandler (int sig);

static int  gKillReceived = 0;
static int  gcount = 0;

int
main (int argc, char *argv[])
{
  dbtag_t     dbtag;
  uint16_t    listenPort;
  int         flags;

  osSetStandardSignals (dbtagSigHandler);

  flags = BDJ4_INIT_NO_DB_LOAD | BDJ4_INIT_NO_DATAFILE_LOAD;
  bdj4startup (argc, argv, NULL, "dt", ROUTE_DBTAG, flags);
  logProcBegin (LOG_PROC, "dbtag");

  dbtag.maxThreads = sysvarsGetNum (SVL_NUM_PROC);
  dbtag.threads = malloc (sizeof (dbthread_t) * dbtag.maxThreads);
  dbtag.running = false;
  dbtag.numActiveThreads = 0;
  dbtag.iterations = 0;
  dbtag.threadActiveSum = 0;
  dbtag.progstate = progstateInit ("dbtag");
  for (int i = 0; i < dbtag.maxThreads; ++i) {
    dbtag.threads [i].state = DBTAG_T_STATE_INIT;
    dbtag.threads [i].idx = i;
    dbtag.threads [i].count = 0;
    dbtag.threads [i].fn = NULL;
    dbtag.threads [i].data = NULL;
  }
  dbtag.fileQueue = queueAlloc (free);
  dbtag.received = 0;
  dbtag.sent = 0;
  dbtag.maxqueuelen = 0;

  progstateSetCallback (dbtag.progstate, STATE_CONNECTING,
      dbtagConnectingCallback, &dbtag);
  progstateSetCallback (dbtag.progstate, STATE_WAIT_HANDSHAKE,
      dbtagHandshakeCallback, &dbtag);
  progstateSetCallback (dbtag.progstate, STATE_STOPPING,
      dbtagStoppingCallback, &dbtag);
  progstateSetCallback (dbtag.progstate, STATE_CLOSING,
      dbtagClosingCallback, &dbtag);

  for (bdjmsgroute_t i = ROUTE_NONE; i < ROUTE_MAX; ++i) {
    dbtag.processes [i] = NULL;
  }

  dbtag.conn = connInit (ROUTE_DBTAG);

  listenPort = bdjvarsGetNum (BDJVL_DBTAG_PORT);
  sockhMainLoop (listenPort, dbtagProcessMsg, dbtagProcessing, &dbtag);

  while (progstateShutdownProcess (dbtag.progstate) != STATE_CLOSED) {
    mssleep (50);
  }
  progstateFree (dbtag.progstate);

  logProcEnd (LOG_PROC, "dbtag", "");
  logEnd ();
  return 0;
}

/* internal routines */

static int
dbtagProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  dbtag_t      *dbtag;

  dbtag = (dbtag_t *) udata;

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_DBTAG: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          connProcessHandshake (dbtag->conn, routefrom);
          connConnectResponse (dbtag->conn, routefrom);
          break;
        }
        case MSG_SOCKET_CLOSE: {
          connDisconnect (dbtag->conn, routefrom);
          break;
        }
        case MSG_EXIT_REQUEST: {
          logMsg (LOG_SESS, LOG_IMPORTANT, "got exit request");
          gKillReceived = 0;
          progstateShutdownProcess (dbtag->progstate);
          logProcEnd (LOG_PROC, "dbtagProcessMsg", "req-exit");
          return 1;
        }
        case MSG_DB_FILE_CHK: {
          dbtagProcessFileMsg (dbtag, args);
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
dbtagProcessing (void *udata)
{
  dbtag_t       * dbtag = NULL;
  char          sbuff [BDJMSG_MAX_ARGS];
  dbidx_t       count;

  dbtag = (dbtag_t *) udata;

  if (! progstateIsRunning (dbtag->progstate)) {
    progstateProcess (dbtag->progstate);
    if (gKillReceived) {
      logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
    }
    return gKillReceived;
  }

  for (int i = 0; i < dbtag->maxThreads; ++i) {
    if (dbtag->threads [i].state == DBTAG_T_STATE_HAVE_DATA) {
      pthread_join (dbtag->threads [i].thread, NULL);
      snprintf (sbuff, sizeof (sbuff), "%s%c%s",
          dbtag->threads [i].fn, MSG_ARGS_RS, dbtag->threads [i].data);
      ++dbtag->sent;
      connSendMessage (dbtag->conn, ROUTE_DBUPDATE, MSG_DB_FILE_TAGS, sbuff);
      if (dbtag->threads [i].fn != NULL) {
        free (dbtag->threads [i].fn);
      }
      dbtag->threads [i].fn = NULL;
      if (dbtag->threads [i].data != NULL) {
        free (dbtag->threads [i].data);
      }
      dbtag->threads [i].data = NULL;
      dbtag->threads [i].state = DBTAG_T_STATE_INIT;
      --dbtag->numActiveThreads;
    }
  }

  if (dbtag->running) {
    count = queueGetCount (dbtag->fileQueue);
    if (count > dbtag->maxqueuelen) {
      dbtag->maxqueuelen = count;
    }
    while (queueGetCount (dbtag->fileQueue) > 0 &&
        dbtag->numActiveThreads < dbtag->maxThreads) {
      ++dbtag->iterations;
      if (dbtag->iterations > dbtag->maxThreads) {
        dbtag->threadActiveSum += dbtag->numActiveThreads;
      }
      for (int i = 0; i < dbtag->maxThreads; ++i) {
        if (dbtag->threads [i].state == DBTAG_T_STATE_INIT) {
          char    *fn;

          fn = queuePop (dbtag->fileQueue);
          if (fn == NULL) {
            break;
          }

          dbtag->threads [i].state = DBTAG_T_STATE_ACTIVE;
          ++dbtag->numActiveThreads;
          if (dbtag->threads [i].fn != NULL) {
            free (dbtag->threads [i].fn);
            dbtag->threads [i].fn = NULL;
          }
          /* fn is already allocated */
          dbtag->threads [i].fn = fn;
          logMsg (LOG_DBG, LOG_DBUPDATE, "process: %s", fn);
          ++gcount;
          dbtag->threads [i].count = gcount;
          pthread_create (&dbtag->threads [i].thread, NULL, dbtagProcessFile, &dbtag->threads [i]);
        }
      }
    }

    if (queueGetCount (dbtag->fileQueue) == 0 &&
        dbtag->numActiveThreads == 0) {
      logMsg (LOG_DBG, LOG_IMPORTANT, "-- queue empty: %ld ms", mstimeend (&dbtag->starttm));
      logMsg (LOG_DBG, LOG_IMPORTANT, "     received: %ld", dbtag->received);
      logMsg (LOG_DBG, LOG_IMPORTANT, "         sent: %ld", dbtag->sent);
      logMsg (LOG_DBG, LOG_IMPORTANT, "max queue len: %ld", dbtag->maxqueuelen);
      logMsg (LOG_DBG, LOG_IMPORTANT, "average num threads active: %.2f",
          (double) dbtag->threadActiveSum /
          (double) (dbtag->iterations - dbtag->maxThreads));
      progstateShutdownProcess (dbtag->progstate);
      dbtag->running = false;
    }
  }

  if (gKillReceived) {
    logMsg (LOG_SESS, LOG_IMPORTANT, "got kill signal");
  }
  return gKillReceived;
}

static bool
dbtagConnectingCallback (void *tdbtag, programstate_t programState)
{
  dbtag_t *dbtag = tdbtag;
  bool    rc = false;

  logProcBegin (LOG_PROC, "dbtagConnectingCallback");

  if (! connIsConnected (dbtag->conn, ROUTE_DBUPDATE)) {
    connConnect (dbtag->conn, ROUTE_DBUPDATE);
  }

  if (connIsConnected (dbtag->conn, ROUTE_DBUPDATE)) {
    rc = true;
  }

  logProcEnd (LOG_PROC, "dbtagConnectingCallback", "");
  return rc;
}

static bool
dbtagHandshakeCallback (void *tdbtag, programstate_t programState)
{
  dbtag_t    *dbtag = tdbtag;
  bool          rc = false;

  logProcBegin (LOG_PROC, "dbtagHandshakeCallback");

  if (connHaveHandshake (dbtag->conn, ROUTE_DBUPDATE)) {
    rc = true;
  }
  logProcEnd (LOG_PROC, "dbtagHandshakeCallback", "");
  return rc;
}

static bool
dbtagStoppingCallback (void *tdbtag, programstate_t programState)
{
  dbtag_t    *dbtag = tdbtag;

  logProcBegin (LOG_PROC, "dbtagStoppingCallback");
  connDisconnectAll (dbtag->conn);
  logProcEnd (LOG_PROC, "dbtagStoppingCallback", "");
  return true;
}

static bool
dbtagClosingCallback (void *tdbtag, programstate_t programState)
{
  dbtag_t    *dbtag = tdbtag;

  logProcBegin (LOG_PROC, "dbtagClosingCallback");
  bdj4shutdown (ROUTE_DBTAG, NULL);

  free (dbtag->threads);
  queueFree (dbtag->fileQueue);
  connFree (dbtag->conn);

  logProcEnd (LOG_PROC, "dbtagClosingCallback", "");
  return true;
}

static void
dbtagProcessFileMsg (dbtag_t *dbtag, char *args)
{
  if (! dbtag->running) {
    dbtag->running = true;
    mstimestart (&dbtag->starttm);
  }
  ++dbtag->received;
  queuePush (dbtag->fileQueue, strdup (args));
}

static void *
dbtagProcessFile (void *tdbthread)
{
  dbthread_t    * dbthread = tdbthread;


  dbthread->data = audiotagReadTags (dbthread->fn);
  dbthread->state = DBTAG_T_STATE_HAVE_DATA;
  pthread_exit (NULL);
  return NULL;
}

static void
dbtagSigHandler (int sig)
{
  gKillReceived = 1;
}
