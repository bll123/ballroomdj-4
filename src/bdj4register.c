#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdjmsg.h"
#include "bdjvars.h"
#include "conn.h"
#include "lock.h"
#include "pathbld.h"
#include "sockh.h"
#include "sysvars.h"
#include "tmutil.h"

typedef struct {
  conn_t  *conn;
  int     processcount;
  int     port;
  bool    started : 1;
} bdj4reg_t;

static int  regProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *args, void *udata);
static int  regProcessing (void *udata);
static void regSendPort (bdj4reg_t *reg, bdjmsgroute_t routefrom);

int
main (int argc, char *argv [])
{
  int         c = 0;
  int         option_index = 0;
  bool        isbdj4 = false;
  uint16_t    listenPort;
  bdj4reg_t   reg;
  int         rc;

  static struct option bdj_options [] = {
    { "bdj4register", no_argument,      NULL,   0 },
    { "bdj4",         no_argument,      NULL,   'B' },
    /* ignored */
    { "nodetach",     no_argument,      NULL,   0 },
    { "debugself",    no_argument,      NULL,   0 },
    { "msys",         no_argument,      NULL,   0 },
    { "theme",        required_argument,NULL,   0 },
    { NULL,         0,                  NULL,   0 }
  };

  while ((c = getopt_long_only (argc, argv, "g:r:u:", bdj_options, &option_index)) != -1) {
    switch (c) {
      case 'B': {
        isbdj4 = true;
        break;
      }
      default: {
        break;
      }
    }
  }

  if (! isbdj4) {
    fprintf (stderr, "not started with launcher\n");
    exit (1);
  }

  reg.processcount = 0;

  sysvarsInit (argv [0]);
  bdjvarsInit ();

  /* the registration lock is static */
  rc = lockAcquire (lockName (ROUTE_REGISTER), PATHBLD_MP_NONE);
  if (rc < 0) {
    bdjvarsCleanup ();
    exit (0);
  }

  listenPort = bdjvarsGetNum (BDJVL_REGISTER_PORT);
  reg.port = listenPort + 1;
  reg.conn = connInit (ROUTE_REGISTER);
  reg.started = true;

  sockhMainLoop (listenPort, regProcessMsg, regProcessing, &reg);

  bdjvarsCleanup ();
  rc = lockRelease (lockName (ROUTE_REGISTER), PATHBLD_MP_NONE);
  return 0;
}

static int
regProcessMsg (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *args, void *udata)
{
  bdj4reg_t  *reg = udata;

  switch (route) {
    case ROUTE_NONE:
    case ROUTE_REGISTER: {
      switch (msg) {
        case MSG_REGISTER: {
          reg->started = false;
          ++reg->processcount;
fprintf (stderr, "reg-count: %d\n", reg->processcount);
          regSendPort (reg, routefrom);
          break;
        }
        case MSG_REGISTER_EXIT: {
          reg->started = false;
          --reg->processcount;
fprintf (stderr, "reg-count: %d\n", reg->processcount);
          break;
        }
        default: {
          break;
        }
      }
    }
    default: {
      break;
    }
  }
  return 0;
}

static int
regProcessing (void *udata)
{
  bdj4reg_t   *reg = udata;
  bool        stop = false;

  if (! reg->started && reg->processcount == 0) {
    stop = true;
  }

  return stop;
}

static void
regSendPort (bdj4reg_t *reg, bdjmsgroute_t routefrom)
{
  char    tbuff [40];

  connConnect (reg->conn, routefrom);
  while (! connIsConnected (reg->conn, routefrom)) {
    mssleep (10);
    connConnect (reg->conn, routefrom);
  }
  snprintf (tbuff, sizeof (tbuff), "%d", reg->port);
  connSendMessage (reg->conn, routefrom, MSG_REGISTER_PORT, tbuff);
  connDisconnect (reg->conn, routefrom);
  /* and bump the port value ready for the next connection */
  reg->port += BDJVL_NUM_PORTS;
}
