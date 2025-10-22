/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "dbusi.h"
#include "fileop.h"
#include "mdebug.h"
#include "osenv.h"
#include "osprocess.h"
#include "osutils.h"

static uint32_t     kdepowercookie = 0;
static uint32_t     kdesscookie = 0;
static uint32_t     kdenotcookie = 0;

static void osSuspendSleepKDE (void);
static void osResumeSleepKDE (void);

void
osSuspendSleep (void)
{
  char    temp [50];

  osGetEnv ("XDG_SESSION_DESKTOP", temp, sizeof (temp));
  if (strcmp (temp, "KDE") == 0) {
    osSuspendSleepKDE ();
  } else {
    const char  *script;

    script = bdjoptGetStr (OPT_M_STARTUP_SCRIPT);
    if (script != NULL &&
        *script &&
        fileopFileExists (script)) {
      const char  *targv [2];

      /* the startup script can be an absolute path or a relative path */
      /* if it is a relative path, it is assumed to be from the main dir */
      /* which is the current location, so no special processing is needed */
      targv [0] = script;
      targv [1] = NULL;
      osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
    }
  }
  return;
}

void
osResumeSleep (void)
{
  char    temp [50];

  osGetEnv ("XDG_SESSION_DESKTOP", temp, sizeof (temp));
  if (strcmp (temp, "KDE") == 0) {
    osResumeSleepKDE ();
  } else {
    const char  *script;

    script = bdjoptGetStr (OPT_M_SHUTDOWN_SCRIPT);
    if (script != NULL &&
        *script &&
        fileopFileExists (script)) {
      const char  *targv [2];

      /* the shutdown script can be an absolute path or a relative path */
      /* if it is a relative path, it is assumed to be from the main dir */
      /* which is the current location, so no special processing is needed */
      targv [0] = script;
      targv [1] = NULL;
      osProcessStart (targv, OS_PROC_DETACH, NULL, NULL);
    }
  }

  return;
}

/* internal routines */

static void
osSuspendSleepKDE (void)
{
  uint32_t      rval = 0;
  dbus_t        *dbus = NULL;

  /* https://github.com/KDE/kde-cli-tools/blob/master/kdeinhibit/main.cpp */
  /*
   * org.freedesktop.PowerManagement.Inhibit
   * /org/freedesktop/PowerManagement/Inhibit
   * org.freedesktop.PowerManagement.Inhibit
   * Inhibit (ss) returns u
   * UnInhibit u (cookie)
   */
  /*
   * org.freedesktop.ScreenSaver
   * /org/freedesktop/ScreenSaver
   * org.freedesktop.ScreenSaver
   * Inhibit (ss) returns u
   * UnInhibit u (cookie)
   */

  dbus = dbusConnInit ();

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(ss)", BDJ4_NAME, "playback");
  dbusMessage (dbus,
      "org.freedesktop.PowerManagement.Inhibit",
      "/org/freedesktop/PowerManagement/Inhibit",
      "org.freedesktop.PowerManagement.Inhibit",
      "Inhibit");
  dbusResultGet (dbus, &rval, NULL);
  kdepowercookie = rval;

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(ss)", BDJ4_NAME, "playback");
  dbusMessage (dbus,
      "org.freedesktop.ScreenSaver",
      "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver",
      "Inhibit");
  dbusResultGet (dbus, &rval, NULL);
  kdesscookie = rval;

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(ss)", BDJ4_NAME, "playback");
  dbusMessage (dbus,
      "org.freedesktop.Notifications",
      "/org/freedesktop/Notifications",
      "org.freedesktop.Notifications",
      "Inhibit");
  dbusResultGet (dbus, &rval, NULL);
  kdenotcookie = rval;

  dbusConnClose (dbus);

  return;
}


static void
osResumeSleepKDE (void)
{
  dbus_t  *dbus = NULL;

  dbus = dbusConnInit ();

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(u)", kdepowercookie);
  dbusMessage (dbus,
      "org.freedesktop.PowerManagement.Inhibit",
      "/org/freedesktop/PowerManagement/Inhibit",
      "org.freedesktop.PowerManagement.Inhibit",
      "UnInhibit");

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(u)", kdesscookie);
  dbusMessage (dbus,
      "org.freedesktop.ScreenSaver",
      "/org/freedesktop/ScreenSaver",
      "org.freedesktop.ScreenSaver",
      "UnInhibit");

  dbusMessageInit (dbus);
  dbusMessageSetData (dbus, "(u)", kdenotcookie);
  dbusMessage (dbus,
      "org.freedesktop.Notifications",
      "/org/freedesktop/Notifications",
      "org.freedesktop.Notifications",
      "UnInhibit");

  dbusConnClose (dbus);

  return;
}

