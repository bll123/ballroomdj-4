#include "config.h"
#include "configssl.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>

#include "mongoose.h"

#include "bdjmsg.h"
#include "bdjopt.h"
#include "bdjvars.h"
#include "lock.h"
#include "log.h"
#include "process.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"
#include "websrv.h"

typedef struct {
  long            tval1;
  Sock_t          mainSock;
  programstate_t  programState;
  mstime_t        tmstart;
  uint16_t        port;
  bdjmobilemq_t   type;
  long            tval2;
  char            *name;
  long            tval3;
  char            *title;
  long            tval4;
  websrv_t        *websrv;
  char            *marqueeData;
  bool            done;
  bool            mainHandshake : 1;
  long            tval5;
} mobmqdata_t;

static void mmEventHandler (struct mg_connection *c, int ev,
    void *ev_data, void *userdata);
static int  mobilemqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata);
static int  mobilemqProcessing (void *udata);
static void mobilemqSigHandler (int sig);

static int            gKillReceived = 0;
  /* mongoose seems to trash the userdata, so use a global */
static mobmqdata_t    mobmqData;

int
main (int argc, char *argv[])
{
  int             c = 0;
  int             rc = 0;
  int             option_index = 0;
  bdjloglvl_t     loglevel = LOG_IMPORTANT | LOG_MAIN;
  uint16_t        listenPort;
  char            *tval;

  static struct option bdj_options [] = {
    { "debug",      required_argument,  NULL,   'd' },
    { "profile",    required_argument,  NULL,   'p' },
    { "player",     no_argument,        NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  mstimestart (&mobmqData.tmstart);
  processCatchSignals (mobilemqSigHandler);

  sysvarsInit (argv[0]);

  while ((c = getopt_long (argc, argv, "", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'd': {
        if (optarg) {
          loglevel = atoi (optarg);
        }
        break;
      }
      case 'p': {
        if (optarg) {
          sysvarSetNum (SVL_BDJIDX, atol (optarg));
        }
        break;
      }
      default: {
        break;
      }
    }
  }

  logStartAppend ("mobilemarquee", "mm", loglevel);
  logMsg (LOG_SESS, LOG_IMPORTANT, "Using profile %ld", lsysvars [SVL_BDJIDX]);

  rc = lockAcquire (MOBILEMQ_LOCK_FN, DATAUTIL_MP_USEIDX);
  if (rc < 0) {
    logMsg (LOG_DBG, LOG_IMPORTANT, "ERR: mobilemq: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logMsg (LOG_SESS, LOG_IMPORTANT, "ERR: mobilemq: unable to acquire lock: profile: %zd", lsysvars [SVL_BDJIDX]);
    logEnd ();
    exit (0);
  }

  bdjvarsInit ();
  bdjoptInit ();

  mobmqData.type = bdjoptGetNum (OPT_P_MOBILEMARQUEE);
  if (mobmqData.type == MOBILEMQ_OFF) {
    exit (0);
  }

  mobmqData.mainSock = INVALID_SOCKET;
  mobmqData.programState = STATE_INITIALIZING;
  mobmqData.port = bdjoptGetNum (OPT_P_MOBILEMQPORT);
  mobmqData.name = NULL;
  tval = bdjoptGetData (OPT_P_MOBILEMQTAG);
  if (tval != NULL) {
    mobmqData.name = strdup (tval);
  }
  mobmqData.title = NULL;
  if (tval != NULL) {
    mobmqData.title = strdup (tval);
  }
  mobmqData.websrv = NULL;
  mobmqData.marqueeData = NULL;
  mobmqData.done = false;
  mobmqData.mainHandshake = false;

  mobmqData.tval1 = 0x11223344;
  mobmqData.tval2 = 0x11223344;
  mobmqData.tval3 = 0x11223344;
  mobmqData.tval4 = 0x11223344;
  mobmqData.tval5 = 0x11223344;

  mobmqData.websrv = websrvInit (mobmqData.port, mmEventHandler,
      &mobmqData.done, NULL);

  listenPort = bdjvarsl [BDJVL_MOBILEMQ_PORT];
  sockhMainLoop (listenPort, mobilemqProcessMsg, mobilemqProcessing, &mobmqData);

  websrvFree (mobmqData.websrv);
  if (mobmqData.name != NULL) {
    free (mobmqData.name);
  }
  if (mobmqData.title != NULL) {
    free (mobmqData.title);
  }
  if (mobmqData.marqueeData != NULL) {
    free (mobmqData.marqueeData);
  }
  bdjoptFree ();
  bdjvarsCleanup ();
  logEnd ();
  lockRelease (MOBILEMQ_LOCK_FN, DATAUTIL_MP_USEIDX);
  return 0;
}

/* internal routines */

static void
mmEventHandler (struct mg_connection *c, int ev, void *ev_data, void *userdata)
{
  char          *data = NULL;
  char          *title = NULL;
  char          tbuff [400];

  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (mg_http_match_uri (hm, "/mmupdate")) {
      data = mobmqData.marqueeData;
      title = mobmqData.title;
      if (data == NULL) {
        if (title == NULL) {
          title = "";
        }
        snprintf (tbuff, sizeof (tbuff),
            "{ "
            "\"title\" : \"%s\","
            "\"current\" : \"Not Playing\","
            "\"skip\" : \"true\""
            "}",
            title);
        data = tbuff;
      }
      mg_http_reply (c, 200, "Content-Type: application/json\r\n",
          "%s\r\n", data);
    } else if (mg_http_match_uri (hm, "#.key") ||
        mg_http_match_uri (hm, "#.crt") ||
        mg_http_match_uri (hm, "#.pem") ||
        mg_http_match_uri (hm, "#.csr")) {
      mg_http_reply (c, 403, NULL, "");
    } else {
      struct mg_http_serve_opts opts = { .root_dir = "http" };
      mg_http_serve_dir (c, hm, &opts);
    }
  }
}

static int
mobilemqProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  mobmqdata_t     *mobmqData = udata;

  logMsg (LOG_DBG, LOG_MSGS, "got: from: %ld route: %ld msg:%ld args:%s",
      routefrom, route, msg, args);

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_MOBILEMQ: {
      switch (msg) {
        case MSG_HANDSHAKE: {
          mobmqData->mainHandshake = true;
          break;
        }
        case MSG_EXIT_REQUEST: {
          gKillReceived = 0;
          sockhSendMessage (mobmqData->mainSock, ROUTE_MOBILEMQ, ROUTE_MAIN,
              MSG_SOCKET_CLOSE, NULL);
          sockClose (mobmqData->mainSock);
          mobmqData->mainSock = INVALID_SOCKET;
          return 1;
        }
        case MSG_MARQUEE_DATA: {
          if (mobmqData->marqueeData != NULL) {
            free (mobmqData->marqueeData);
          }
          mobmqData->marqueeData = strdup (args);
          assert (mobmqData->marqueeData != NULL);
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
mobilemqProcessing (void *udata)
{
  mobmqdata_t     *mobmqData = udata;
  websrv_t        *websrv = mobmqData->websrv;


  if (mobmqData->programState == STATE_INITIALIZING) {
    mobmqData->programState = STATE_CONNECTING;
  }

  if (mobmqData->programState == STATE_CONNECTING &&
      mobmqData->mainSock == INVALID_SOCKET) {
    int       err;
    uint16_t  mainPort;

    mainPort = bdjvarsl [BDJVL_MAIN_PORT];
    mobmqData->mainSock = sockConnect (mainPort, &err, 1000);
    if (mobmqData->mainSock != INVALID_SOCKET) {
      sockhSendMessage (mobmqData->mainSock, ROUTE_MOBILEMQ, ROUTE_MAIN,
          MSG_HANDSHAKE, NULL);
      mobmqData->programState = STATE_WAIT_HANDSHAKE;
    }
  }

  if (mobmqData->programState == STATE_WAIT_HANDSHAKE) {
    if (mobmqData->mainHandshake) {
      mobmqData->programState = STATE_RUNNING;
      logMsg (LOG_SESS, LOG_IMPORTANT, "running: time-to-start: %ld ms", mstimeend (&mobmqData->tmstart));
    }
  }

  if (mobmqData->programState != STATE_RUNNING) {
      /* all of the processing that follows requires a running state */
    return gKillReceived;
  }

  websrvProcess (websrv);
  return gKillReceived;
}

static void
mobilemqSigHandler (int sig)
{
  gKillReceived = 1;
}
