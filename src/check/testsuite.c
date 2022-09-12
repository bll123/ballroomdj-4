#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#include "bdjmsg.h"
#include "bdjstring.h"
#include "conn.h"
#include "fileop.h"
#include "istring.h"
#include "localeutil.h"
#include "log.h"
#include "osrandom.h"
#include "procutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"

static slist_t  *groutetxt = NULL;
static slist_t  *gmsgtxt = NULL;

typedef struct {
  int     msgsent;
  int     msgrcvd;
  int     chkcount;
  int     chkfail;
} results_t;

typedef enum {
  TS_OK,
  TS_BAD_ROUTE,
  TS_BAD_MSG,
  TS_UNKNOWN,
  TS_NOT_IMPLEMENTED,
} ts_return_t;

static void testInit (void);
static void testFree (void);
static void clearResults (results_t *results);
static void tallyResults (results_t *results, results_t *gresults);
static void printResults (const char *tname, results_t *results);
static int  processMsg (conn_t *conn, const char *tcmd, results_t *results);
static int  processChk (conn_t *conn, const char *tcmd, results_t *results);
static int  processWaitChk (conn_t *conn, const char *tcmd, results_t *results);
static int  processSleep (const char *tcmd, results_t *results);
static void makeConnection (conn_t *conn, bdjmsgroute_t route);

int
main (int argc, char *argv [])
{
  FILE        *fh = NULL;
  char        tcmd [200];
  char        tname [80];
  results_t   results;
  results_t   gresults;
  ts_return_t ok;
  int         lineno;
  char        *tmsg = "";
  conn_t      *conn = NULL;
  const char  *targv [5];
  int         targc = 0;
  procutil_t  *processes [ROUTE_MAX];

  sRandom ();
  sysvarsInit (argv [0]);
  istringInit (sysvarsGetStr (SV_LOCALE));
  localeInit ();

  /* must come after istringInit */
  testInit ();

  if (chdir (sysvarsGetStr (SV_BDJ4DATATOPDIR)) < 0) {
    fprintf (stderr, "Unable to chdir: %s\n", sysvarsGetStr (SV_BDJ4DATATOPDIR));
    exit (1);
  }

  logStart ("testsuite", "ts", LOG_ALL);

  procutilInitProcesses (processes);
  conn = connInit (ROUTE_TEST_SUITE);

  /* start main */
  targc = 0;
  targv [targc++] = "--nomarquee";
  targv [targc++] = NULL;
  processes [ROUTE_MAIN] = procutilStartProcess (
        ROUTE_MAIN, "bdj4main", PROCUTIL_DETACH, targv);

  makeConnection (conn, ROUTE_MAIN);

  lineno = 1;
  clearResults (&results);
  clearResults (&gresults);

  fh = fopen ("test-templates/test-script.txt", "r");
  while (fgets (tcmd, sizeof (tcmd), fh) != NULL) {
    if (*tcmd == '#') {
      ++lineno;
      continue;
    }
    if (*tcmd == '\n') {
      ++lineno;
      continue;
    }
    stringTrim (tcmd);

    ok = TS_UNKNOWN;
    if (strncmp (tcmd, "name", 4) == 0) {
      strlcpy (tname, tcmd + 5, sizeof (tname));
      clearResults (&results);
      ok = TS_OK;
    }
    if (strncmp (tcmd, "msg", 3) == 0) {
      ok = processMsg (conn, tcmd, &results);
    }
    if (strncmp (tcmd, "chk", 3) == 0) {
      ok = processChk (conn, tcmd, &results);
    }
    if (strncmp (tcmd, "waitchk", 7) == 0) {
      ok = processWaitChk (conn, tcmd, &results);
    }
    if (strncmp (tcmd, "mssleep", 7) == 0) {
      ok = processSleep (tcmd, &results);
    }
    if (strncmp (tcmd, "end", 3) == 0) {
      printResults (tname, &results);
      tallyResults (&results, &gresults);
      clearResults (&results);
      ok = TS_OK;
    }

    tmsg = "unknown";
    switch (ok) {
      case TS_OK: {
        tmsg = "ok";
        break;
      }
      case TS_BAD_ROUTE: {
        tmsg = "bad route";
        break;
      }
      case TS_BAD_MSG: {
        tmsg = "bad msg";
        break;
      }
      case TS_UNKNOWN: {
        tmsg = "unknown cmd";
        break;
      }
      case TS_NOT_IMPLEMENTED: {
        tmsg = "not implemented";
        break;
      }
    }
    if (ok != TS_OK) {
      fprintf (stdout, "%d: %s (%s)\n", lineno, tmsg, tcmd);
    }
    ++lineno;
  }
  fclose (fh);

  printResults ("Final", &gresults);

  localeCleanup ();
  istringCleanup ();
  logEnd ();

  if (fileopFileExists ("core")) {
    fprintf (stdout, "core dumped\n");
  }

  testFree ();
  return 0;
}

static void
testInit (void)
{
  if (groutetxt != NULL) {
    return;
  }

  groutetxt = slistAlloc ("ts-route-txt", LIST_UNORDERED, NULL);
  slistSetSize (groutetxt, ROUTE_MAX);
  for (int i = 0; i < ROUTE_MAX; ++i) {
    slistSetNum (groutetxt, bdjmsgroutetxt [i], i);
  }
  slistSort (groutetxt);

  gmsgtxt = slistAlloc ("ts-msg-txt", LIST_UNORDERED, NULL);
  slistSetSize (gmsgtxt, MSG_MAX);
  for (int i = 0; i < MSG_MAX; ++i) {
    slistSetNum (gmsgtxt, bdjmsgtxt [i], i);
  }
  slistSort (gmsgtxt);
}

static void
testFree (void)
{
  if (groutetxt != NULL) {
    slistFree (groutetxt);
    slistFree (gmsgtxt);
    groutetxt = NULL;
    gmsgtxt = NULL;
  }
}

static void
clearResults (results_t *results)
{
  results->msgsent = 0;
  results->msgrcvd = 0;
  results->chkcount = 0;
  results->chkfail = 0;
}

static void
tallyResults (results_t *results, results_t *gresults)
{
  gresults->msgsent += results->msgsent;
  gresults->msgrcvd += results->msgrcvd;
  gresults->chkcount += results->chkcount;
  gresults->chkfail += results->chkfail;
}

static void
printResults (const char *tname, results_t *results)
{
  fprintf (stdout, "-- %s %d %d %d %d\n", tname,
      results->msgsent, results->msgrcvd, results->chkcount, results->chkfail);
}


static int
processMsg (conn_t *conn, const char *tcmd, results_t *results)
{
  char  *tstr;
  char  *p;
  char  *tokstr;
  int   route;
  int   msg;

  tstr = strdup (tcmd);
  p = strtok_r (tstr, " ", &tokstr);
  /* 'msg' */
  p = strtok_r (NULL, " ", &tokstr);
  stringAsciiToUpper (p);
  route = slistGetNum (groutetxt, p);
  if (route < 0) {
    free (tstr);
    return TS_BAD_ROUTE;
  }
  p = strtok_r (NULL, " ", &tokstr);
  stringAsciiToUpper (p);
  msg = slistGetNum (gmsgtxt, p);
  if (msg < 0) {
    free (tstr);
    return TS_BAD_MSG;
  }
  p = strtok_r (NULL, " ", &tokstr);

  ++results->msgsent;
  connSendMessage (conn, route, msg, p);
  free (tstr);
  return TS_OK;
}

static int
processChk (conn_t *conn, const char *tcmd, results_t *results)
{
  return TS_NOT_IMPLEMENTED;
}

static int
processWaitChk (conn_t *conn, const char *tcmd, results_t *results)
{
  return TS_NOT_IMPLEMENTED;
}

static int
processSleep (const char *tcmd, results_t *results)
{
  char  *tstr;
  char  *p;
  char  *tokstr;

  tstr = strdup (tcmd);
  p = strtok_r (tstr, " ", &tokstr);
  p = strtok_r (NULL, " ", &tokstr);
  fprintf (stdout, "   sleeping %s\n", p);
  mssleep (atol (p));
  free (tstr);
  return TS_OK;
}


static void
makeConnection (conn_t *conn, bdjmsgroute_t route)
{
  int   count;

  count = 0;
  while (! connIsConnected (conn, route)) {
    connConnect (conn, route);
    if (count > 5) {
      break;
    }
    mssleep (10);
    ++count;
  }
}

