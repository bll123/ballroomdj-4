/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#if __linux__

#include <glib.h>

#include "dbusi.h"

enum {
  MAXSZ = 80,
};

static void getstr (const char *prompt, char *str, size_t sz);
static void getstd (char *bus, char *orgpath, char *intfc, char *method);
#endif /* __linux__ */

int
main (int argc, char *argv [])
{
#if __linux__
  GMainContext *ctx;
  dbus_t    *dbus;
  char      cmd [MAXSZ];
  char      bus [MAXSZ];
  char      orgpath [MAXSZ];
  char      intfc [MAXSZ];
  char      method [MAXSZ];
  char      data [MAXSZ];
  bool      rc;

  /* because this utility does not run under gtk, */
  /* need a context to handle dbus events */
  ctx = g_main_context_default ();

  dbus = dbusConnInit ();
  if (dbus == NULL) {
    fprintf (stderr, "could not connect\n");
    return 1;
  }

  while (1) {
    getstr ("cmd", cmd, sizeof (cmd));

    if (strcmp (cmd, "exit") == 0 ||
        strcmp (cmd, "quit") == 0) {
      break;
    }
    if (strcmp (cmd, "init") == 0) {
      dbusMessageInit (dbus);
    }
    if (strcmp (cmd, "data") == 0) {
      getstr ("data", data, sizeof (data));
      dbusMessageSetDataString (dbus, data, NULL);
    }
    if (strcmp (cmd, "call") == 0) {
      getstd (bus, orgpath, intfc, method);
      if (! *bus || ! *orgpath || ! *intfc || ! *method) {
        continue;
      }

      dbusMessage (dbus, bus, orgpath, intfc, method);
    }
    if (strcmp (cmd, "named") == 0) {
      int   count;

      getstr ("name", data, sizeof (data));
      dbusConnectAcquireName (dbus, data, "org.mpris.MediaPlayer2");
      count = 0;
      rc = dbusCheckAcquireName (dbus);
      while (! rc && count < 400) {
        g_main_context_iteration (ctx, false);
        rc = dbusCheckAcquireName (dbus);
        ++count;
      }
      fprintf (stderr, "acquire: rc: %d count: %d\n", rc, count);
    }
  }

  dbusConnClose (dbus);
  g_main_context_unref (ctx);
#endif /* __linux__ */
  return 0;
}

#if __linux__

static void
getstr (const char *prompt, char *str, size_t sz)
{
  size_t    len;

  fprintf (stderr, "%s: ", prompt);
  fflush (stderr);
  *str = '\0';
  (void) ! fgets (str, sz, stdin);
  len = strlen (str);
  --len;
  str [len] = '\0';
  fprintf (stderr, " %s\n", str);
}

static void
getstd (char *bus, char *orgpath, char *intfc, char *method)
{
  getstr ("bus", bus, MAXSZ);
  getstr ("orgpath", orgpath, MAXSZ);
  getstr ("intfc", intfc, MAXSZ);
  getstr ("method", method, MAXSZ);
}

#endif /* __linux__ */
