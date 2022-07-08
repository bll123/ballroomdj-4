#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>

#include "bdj4.h"
#include "bdj4reg.h"
#include "bdjstring.h"
#include "pathbld.h"
#include "osutils.h"
#include "sysvars.h"
#include "tmutil.h"

void
regStart (void)
{
  char        prog [MAXPATHLEN];
  const char  *targv [4];
  int         targc = 0;

  pathbldMakePath (prog, sizeof (prog),
      "bdj4", sysvarsGetStr (SV_OS_EXEC_EXT), PATHBLD_MP_EXECDIR);
  targv [targc++] = prog;
  targv [targc++] = "--bdj4register";
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
}

void
regRegister (conn_t *conn)
{
  connConnect (conn, ROUTE_REGISTER);
  while (! connIsConnected (conn, ROUTE_REGISTER)) {
    mssleep (10);
    connConnect (conn, ROUTE_REGISTER);
  }
  connSendMessage (conn, ROUTE_REGISTER, MSG_REGISTER, NULL);
  connDisconnect (conn, ROUTE_REGISTER);
}

void
regRegisterExit (conn_t *conn)
{
  connConnect (conn, ROUTE_REGISTER);
  while (! connIsConnected (conn, ROUTE_REGISTER)) {
    mssleep (10);
    connConnect (conn, ROUTE_REGISTER);
  }
  connSendMessage (conn, ROUTE_REGISTER, MSG_REGISTER_EXIT, NULL);
  connDisconnect (conn, ROUTE_REGISTER);
}
