#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#if _hdr_winsock2
# include <winsock2.h>
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include "bdj4.h"
#include "bdj4intl.h"
#include "bdjopt.h"
#include "bdjstring.h"
#include "datafile.h"
#include "dirop.h"
#include "filedata.h"
#include "fileop.h"
#include "filemanip.h"
#include "fileutil.h"
#include "localeutil.h"
#include "log.h"
#include "osuiutils.h"
#include "osutils.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"
#include "webclient.h"

/* installation states */
typedef enum {
  INST_INITIALIZE,
  INST_PREPARE,
  INST_WAIT_USER,
  INST_INIT,
  INST_MAKE_TARGET,
  INST_CHDIR,
  INST_CREATE_DIRS,
  INST_COPY_TEMPLATES,
  INST_CREATE_SHORTCUT,
  INST_UPDATE_PROCESS_INIT,
  INST_UPDATE_PROCESS,
  INST_FINISH,
  INST_EXIT,
} installstate_t;

enum {
  INST_CALLBACK_TARGET_DIR,
  INST_CALLBACK_EXIT,
  INST_CALLBACK_INSTALL,
  INST_CALLBACK_MAX,
};

enum {
  INST_TARGET,
};

typedef struct {
  installstate_t  instState;
  UICallback      callbacks [INST_CALLBACK_MAX];
  char            *target;
  char            rundir [20];  // ###
  char            hostname [20];  // ###
  char            home [20];  // ###
  char            dlfname [MAXPATHLEN];
  /* conversion */
  UIWidget        window;
  uientry_t       *targetEntry;
  UIWidget        reinstWidget;
  UICallback      reinstcb;
  UIWidget        feedbackMsg;
  uitextbox_t     *disptb;
  /* flags */
  bool            uiBuilt : 1;
  bool            scrolltoend : 1;
  bool            reinstall : 1;   // ### fix
} installer_t;

#define INST_HL_COLOR "#b16400"
#define INST_SEP_COLOR "#733000"

static void installerBuildUI (installer_t *installer);
static int  installerMainLoop (void *udata);
static bool installerExitCallback (void *udata);
static bool installerCheckDirTarget (void *udata);
static bool installerTargetDirDialog (void *udata);
static int  installerValidateTarget (uientry_t *entry, void *udata);
static bool installerInstallCallback (void *udata);
static bool installerCheckTarget (installer_t *installer, const char *dir);
static void installerSetPaths (installer_t *installer);

static void installerInstInit (installer_t *installer);
static void installerMakeTarget (installer_t *installer);
static void installerChangeDir (installer_t *installer);
static void installerCreateDirs (installer_t *installer);
static void installerCopyTemplates (installer_t *installer);
static void installerCreateShortcut (installer_t *installer);
static void installerUpdateProcessInit (installer_t *installer);
static void installerUpdateProcess (installer_t *installer);

static void installerCleanup (installer_t *installer);
static void installerDisplayText (installer_t *installer, char *pfx, char *txt, bool bold);
static void installerTemplateCopy (const char *dir, const char *from, const char *to);
static void installerFailWorkingDir (installer_t *installer, const char *dir);
static void installerSetTargetDir (installer_t *installer, const char *fn);
static void installerCheckAndFixTarget (char *buff, size_t sz);

int
main (int argc, char *argv[])
{
  installer_t   installer;
  char          buff [MAXPATHLEN];
  char          *uifont;

  buff [0] = '\0';

  uiutilsUIWidgetInit (&installer.window);
  installer.instState = INST_INITIALIZE;
  installer.target = strdup ("");
  installer.uiBuilt = false;
  installer.scrolltoend = false;
  *installer.rundir = '\0';
  *installer.home = '\0';
  *installer.hostname = '\0';
  uiutilsUIWidgetInit (&installer.reinstWidget);
  uiutilsUIWidgetInit (&installer.feedbackMsg);

  installer.targetEntry = uiEntryInit (80, MAXPATHLEN);

  /* the data in sysvars will not be correct.  don't use it.  */
  /* the installer only needs the home, hostname, os info and locale */
  sysvarsInit (argv[0]);
  localeInit ();

  uiUIInitialize ();

  uifont = sysvarsGetStr (SV_FONT_DEFAULT);
  if (uifont == NULL || ! *uifont) {
    uifont = "Arial 12";
  }
  uiSetUIFont (uifont);

  installerBuildUI (&installer);

  while (installer.instState != INST_EXIT) {
    installerMainLoop (&installer);
    mssleep (10);
  }

  /* process any final events */
  uiUIProcessEvents ();

  installerCleanup (&installer);
  logEnd ();
  return 0;
}

static void
installerBuildUI (installer_t *installer)
{
  UIWidget      vbox;
  UIWidget      hbox;
  UIWidget      uiwidget;
  UIWidget      *uiwidgetp;
  char          tbuff [100];
  char          imgbuff [MAXPATHLEN];


  uiutilsUIWidgetInit (&vbox);
  uiutilsUIWidgetInit (&hbox);
  uiutilsUIWidgetInit (&uiwidget);

  strlcpy (imgbuff, "img/bdj4_icon_inst.png", sizeof (imgbuff));
  osuiSetIcon (imgbuff);

  strlcpy (imgbuff, "img/bdj4_icon_inst.svg", sizeof (imgbuff));
  /* CONTEXT: installer: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Installer"), BDJ4_NAME);
  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_EXIT],
      installerExitCallback, installer);
  uiCreateMainWindow (&installer->window,
      &installer->callbacks [INST_CALLBACK_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (&installer->window, 1000, 600);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindow (&installer->window, &vbox);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: installer: where BDJ4 gets installed */
      _("Enter the destination folder where BDJ4 will be installed."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiEntryCreate (installer->targetEntry);
  uiEntrySetValue (installer->targetEntry, installer->target);
  uiwidgetp = uiEntryGetUIWidget (installer->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (installer->targetEntry,
      installerValidateTarget, installer, UIENTRY_DELAYED);

  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_TARGET_DIR],
      installerTargetDirDialog, installer);
  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_TARGET_DIR],
      "", NULL);
  uiButtonSetImageIcon (&uiwidget, "folder");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: installer: checkbox: re-install BDJ4 */
  uiCreateCheckButton (&installer->reinstWidget, _("Re-Install"),
      installer->reinstall);
  uiBoxPackStart (&hbox, &installer->reinstWidget);
  uiutilsUICallbackInit (&installer->reinstcb, installerCheckDirTarget, installer);
  uiToggleButtonSetCallback (&installer->reinstWidget, &installer->reinstcb);

  uiCreateLabel (&installer->feedbackMsg, "");
  uiLabelSetColor (&installer->feedbackMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &installer->feedbackMsg);

  uiCreateHorizSeparator (&uiwidget);
  uiSeparatorSetColor (&uiwidget, INST_SEP_COLOR);
  uiBoxPackStart (&vbox, &uiwidget);

  /* button box */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_EXIT],
      /* CONTEXT: exits the installer */
      _("Exit"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiutilsUICallbackInit (&installer->callbacks [INST_CALLBACK_INSTALL],
      installerInstallCallback, installer);
  uiCreateButton (&uiwidget,
      &installer->callbacks [INST_CALLBACK_INSTALL],
      /* CONTEXT: installer: start the installation process */
      _("Install"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  installer->disptb = uiTextBoxCreate (200);
  uiTextBoxSetReadonly (installer->disptb);
  uiTextBoxHorizExpand (installer->disptb);
  uiTextBoxVertExpand (installer->disptb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (installer->disptb));

  uiWidgetShowAll (&installer->window);
  installer->uiBuilt = true;
}

static int
installerMainLoop (void *udata)
{
  installer_t *installer = udata;

  uiUIProcessEvents ();

  uiEntryValidate (installer->targetEntry, false);

  if (installer->scrolltoend) {
    uiTextBoxScrollToEnd (installer->disptb);
    installer->scrolltoend = false;
    uiUIProcessEvents ();
    mssleep (10);
    uiUIProcessEvents ();
    /* go through the main loop once more */
    return TRUE;
  }

  switch (installer->instState) {
    case INST_INITIALIZE: {
      installer->instState = INST_PREPARE;
      break;
    }
    case INST_PREPARE: {
      uiEntryValidate (installer->targetEntry, true);
      installer->instState = INST_WAIT_USER;
      break;
    }
    case INST_WAIT_USER: {
      /* do nothing */
      break;
    }
    case INST_INIT: {
      installerInstInit (installer);
      break;
    }
    case INST_MAKE_TARGET: {
      installerMakeTarget (installer);
      break;
    }
    case INST_CHDIR: {
      installerChangeDir (installer);
      break;
    }
    case INST_CREATE_DIRS: {
      installerCreateDirs (installer);

      logStart ("bdj4instalt", "ia",
          LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== alt installer started");
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", installer->target);
      break;
    }
    case INST_COPY_TEMPLATES: {
      installerCopyTemplates (installer);
      break;
    }
    case INST_CREATE_SHORTCUT: {
      installerCreateShortcut (installer);
      break;
    }
    case INST_UPDATE_PROCESS_INIT: {
      installerUpdateProcessInit (installer);
      break;
    }
    case INST_UPDATE_PROCESS: {
      installerUpdateProcess (installer);
      break;
    }
    case INST_FINISH: {
      /* CONTEXT: installer: status message */
      installerDisplayText (installer, "## ",  _("Installation complete."), true);
      installer->instState = INST_PREPARE;
      break;
    }
    case INST_EXIT: {
      break;
    }
  }

  return TRUE;
}

static bool
installerCheckDirTarget (void *udata)
{
  installer_t   *installer = udata;

  uiEntryValidate (installer->targetEntry, true);
  return UICB_CONT;
}

static int
installerValidateTarget (uientry_t *entry, void *udata)
{
  installer_t   *installer = udata;
  const char    *dir;
  bool          exists = false;
  bool          tbool;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_OK;

  if (! installer->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (installer->targetEntry);
  tbool = uiToggleButtonIsActive (&installer->reinstWidget);
  installer->reinstall = tbool;

  exists = fileopIsDirectory (dir);

  if (exists) {
    exists = installerCheckTarget (installer, dir);

    if (exists) {
      if (installer->reinstall) {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Re-install %s."), BDJ4_NAME);
        uiLabelSetText (&installer->feedbackMsg, tbuff);
      } else {
        /* CONTEXT: installer: message indicating the action that will be taken */
        snprintf (tbuff, sizeof (tbuff), _("Updating existing %s installation."), BDJ4_NAME);
        uiLabelSetText (&installer->feedbackMsg, tbuff);
      }
    } else {
      /* CONTEXT: installer: the selected folder exists and is not a BDJ4 installation */
      uiLabelSetText (&installer->feedbackMsg, _("Error: Folder already exists."));
      rc = UIENTRY_ERROR;
    }
  } else {
    /* CONTEXT: installer: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New %s installation."), BDJ4_NAME);
    uiLabelSetText (&installer->feedbackMsg, tbuff);
  }

  if (rc == UIENTRY_OK) {
    installerSetPaths (installer);
  }
  return rc;
}

static bool
installerTargetDirDialog (void *udata)
{
  installer_t *installer = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiDialogCreateSelect (&installer->window,
      /* CONTEXT: installer: dialog title for selecting install location */
      _("Install Location"),
      uiEntryGetValue (installer->targetEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    installerCheckAndFixTarget (tbuff, sizeof (tbuff));
    /* validation gets called again upon set */
    uiEntrySetValue (installer->targetEntry, tbuff);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", installer->target);
  }
  free (selectdata);
  return UICB_CONT;
}

static bool
installerExitCallback (void *udata)
{
  installer_t   *installer = udata;

  installer->instState = INST_EXIT;
  return UICB_CONT;
}

static bool
installerInstallCallback (void *udata)
{
  installer_t *installer = udata;

  if (installer->instState == INST_WAIT_USER) {
    installer->instState = INST_INIT;
  }
  return UICB_CONT;
}

static bool
installerCheckTarget (installer_t *installer, const char *dir)
{
  char        tbuff [MAXPATHLEN];
  bool        exists;

//###
  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      "somewhere", sysvarsGetStr (SV_OS_EXEC_EXT));
  exists = fileopFileExists (tbuff);

  if (! exists) {
    snprintf (tbuff, sizeof (tbuff), "%s/%s/bin/bdj4%s",
        dir, BDJ4_NAME, sysvarsGetStr (SV_OS_EXEC_EXT));
    exists = fileopFileExists (tbuff);
    if (exists) {
      snprintf (tbuff, sizeof (tbuff), "%s/%s", dir, BDJ4_NAME);
    }
  }

  return exists;
}

static void
installerSetPaths (installer_t *installer)
{
  installerSetTargetDir (installer, uiEntryGetValue (installer->targetEntry));
}

/* installation routines */

static void
installerInstInit (installer_t *installer)
{
  bool        exists = false;
  char        tbuff [MAXPATHLEN];

  installerSetPaths (installer);
  installer->reinstall = uiToggleButtonIsActive (&installer->reinstWidget);

  exists = fileopIsDirectory (installer->target);
  if (exists) {
    exists = installerCheckTarget (installer, installer->target);

    if (! exists) {
      /* CONTEXT: installer: command line interface: indicating action */
      printf (_("New %s installation."), BDJ4_NAME);

      /* do not allow an overwrite of an existing directory that is not bdj4 */
      /* CONTEXT: installer: command line interface: status message */
      uiLabelSetText (&installer->feedbackMsg, _("Folder already exists."));

      /* CONTEXT: installer: command line interface: the selected folder exists and is not a BDJ4 installation */
      snprintf (tbuff, sizeof (tbuff), _("Error: Folder %s already exists."),
          installer->target);
      installerDisplayText (installer, "", tbuff, false);
      /* CONTEXT: installer: command line interface: status message */
      installerDisplayText (installer, " * ", _("Installation aborted."), false);
      installer->instState = INST_WAIT_USER;
      return;
    }
  }

  installer->instState = INST_MAKE_TARGET;
}

static void
installerMakeTarget (installer_t *installer)
{
  fileopMakeDir (installer->target);

  installer->instState = INST_CHDIR;
}

static void
installerChangeDir (installer_t *installer)
{
// ### change dir to target

  installer->instState = INST_CREATE_DIRS;
}

static void
installerCreateDirs (installer_t *installer)
{
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Creating folder structure."), false);

  /* create the directories that are not included in the distribution */
  fileopMakeDir ("data");
  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  fileopMakeDir ("tmp");
  fileopMakeDir ("http");

  installer->instState = INST_COPY_TEMPLATES;
}

static void
installerCopyTemplates (installer_t *installer)
{
  char            dir [MAXPATHLEN];
  char            from [MAXPATHLEN];
  char            to [MAXPATHLEN];
  char            tbuff [MAXPATHLEN];
  const char      *fname;
  slist_t         *dirlist;
  slistidx_t      iteridx;
  pathinfo_t      *pi;
  datafile_t      *srdf;
  datafile_t      *qddf;
  datafile_t      *autodf;
  slist_t         *renamelist;


  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Copying template files."), false);

  if (chdir (installer->target)) {
    installerFailWorkingDir (installer, installer->target);
    return;
  }

  renamelist = NULL;

  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-sr.txt");
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-auto.txt");
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  snprintf (tbuff, sizeof (tbuff), "%s/install/%s",
      installer->rundir, "localized-qd.txt");
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);

  snprintf (dir, sizeof (dir), "%s/templates", installer->rundir);

  snprintf (tbuff, sizeof (tbuff), "%s/templates", installer->rundir);
  dirlist = diropBasicDirList (tbuff, NULL);
  slistStartIterator (dirlist, &iteridx);
  while ((fname = slistIterateKey (dirlist, &iteridx)) != NULL) {
    if (strcmp (fname, "qrcode") == 0) {
      continue;
    }
    if (strcmp (fname, "qrcode.html") == 0) {
      continue;
    }
    if (strcmp (fname, "html-list.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "helpdata.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "volintfc.txt") == 0) {
      continue;
    }
    if (strcmp (fname, "playerintfc.txt") == 0) {
      continue;
    }

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/bdj4remote.html");
      installerTemplateCopy (dir, from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/%s", fname);
      installerTemplateCopy (dir, from, to);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, ".html")) {
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "http/%s", fname);
    } else if (pathInfoExtCheck (pi, ".svg")) {
      snprintf (from, sizeof (from), "%s", fname);
      snprintf (to, sizeof (to), "%s/img/%s",
          installer->rundir, fname);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (from, sizeof (from), "%s", fname);

      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".p")) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".m")) {
        snprintf (to, sizeof (to), "data/%s/%s", installer->hostname, tbuff);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        snprintf (to, sizeof (to), "data/%s/profile00/%s",
            installer->hostname, tbuff);
      } else {
        /* one of the localized versions */
        free (pi);
        continue;
      }
    } else if (pathInfoExtCheck (pi, BDJ4_CONFIG_EXT) ||
        pathInfoExtCheck (pi, BDJ4_SEQUENCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PL_DANCE_EXT) ||
        pathInfoExtCheck (pi, BDJ4_PLAYLIST_EXT) ) {

      renamelist = NULL;
      if (strncmp (pi->basename, "automatic", pi->blen) == 0) {
        renamelist = datafileGetList (autodf);
      }
      if (strncmp (pi->basename, "standardrounds", pi->blen) == 0) {
        renamelist = datafileGetList (srdf);
      }
      if (strncmp (pi->basename, "QueueDance", pi->blen) == 0) {
        renamelist = datafileGetList (qddf);
      }

      strlcpy (tbuff, fname, sizeof (tbuff));
      if (renamelist != NULL) {
        char    *tval;

        tval = slistGetStr (renamelist, sysvarsGetStr (SV_LOCALE_SHORT));
        if (tval != NULL) {
          snprintf (tbuff, sizeof (tbuff), "%s%*s", tval, (int) pi->elen,
              pi->extension);
        }
      }

      snprintf (from, sizeof (from), "%s", tbuff);
      if (strncmp (pi->basename, "ds-", 3) == 0) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      }
    } else {
      /* uknown extension, probably a localized file */
      free (pi);
      continue;
    }

    installerTemplateCopy (dir, from, to);

    free (pi);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/img", installer->rundir);

  strlcpy (from, "favicon.ico", sizeof (from));
  snprintf (to, sizeof (to), "http/favicon.ico");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "led_on.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_on.svg");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "led_off.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_off.svg");
  installerTemplateCopy (dir, from, to);

  strlcpy (from, "ballroomdj4.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/ballroomdj4.svg");
  installerTemplateCopy (dir, from, to);

  snprintf (from, sizeof (from), "%s/img/mrc", installer->rundir);
  snprintf (to, sizeof (to), "http/mrc");
  *tbuff = '\0';
  if (isWindows ()) {
    pathWinPath (from, sizeof (from));
    pathWinPath (to, sizeof (to));
    snprintf (tbuff, sizeof (tbuff), "robocopy /e /j /dcopy:DAT /timfix /njh /njs /np /ndl /nfl \"%s\" \"%s\"",
        from, to);
  } else {
    snprintf (tbuff, sizeof (tbuff), "cp -r '%s' '%s'", from, "http");
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "copy files: %s", tbuff);
  system (tbuff);

  if (isMacOS ()) {
    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/macOS-Mojave-dark");
    snprintf (to, sizeof (to), "%s/.themes/macOS-Mojave-dark", installer->home);
    filemanipLinkCopy (from, to);

    snprintf (from, sizeof (from), "../Applications/BDJ4.app/Contents/MacOS/plocal/share/themes/macOS-Mojave-light");
    snprintf (to, sizeof (to), "%s/.themes/macOS-Mojave-light", installer->home);
    filemanipLinkCopy (from, to);
  }

  datafileFree (srdf);
  datafileFree (autodf);
  datafileFree (qddf);

  installer->instState = INST_CREATE_SHORTCUT;
}

static void
installerCreateShortcut (installer_t *installer)
{
  char buff [MAXPATHLEN];

  if (chdir (installer->rundir)) {
    installerFailWorkingDir (installer, installer->rundir);
    return;
  }

  /* CONTEXT: installer: status message */
  installerDisplayText (installer, "-- ", _("Creating shortcut."), false);
  if (isWindows ()) {
    if (! chdir ("install")) {
      snprintf (buff, sizeof (buff), ".\\makeshortcut.bat \"%s\"",
          installer->rundir);
      system (buff);
      chdir (installer->rundir);
    }
  }
  if (isMacOS ()) {
#if _lib_symlink
    /* on macos, the startup program must be a gui program, otherwise */
    /* the dock icon is not correct */
    /* this must exist and match the name of the app */
    symlink ("bin/bdj4g", "BDJ4");
    /* desktop shortcut */
    snprintf (buff, sizeof (buff), "%s/Desktop/BDJ4.app", installer->home);
    symlink (installer->target, buff);
#endif
  }
  if (isLinux ()) {
    snprintf (buff, sizeof (buff), "./install/linuxshortcut.sh '%s'",
        installer->rundir);
    system (buff);
  }

  installer->instState = INST_UPDATE_PROCESS;
}

static void
installerUpdateProcessInit (installer_t *installer)
{
  char  buff [MAXPATHLEN];

  /* CONTEXT: installer: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  installerDisplayText (installer, "-- ", buff, false);
  installer->instState = INST_UPDATE_PROCESS;
}

static void
installerUpdateProcess (installer_t *installer)
{
  char  tbuff [MAXPATHLEN];
  int   targc = 0;
  const char  *targv [10];

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      installer->rundir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4updater";

  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
// ### fix
//  if (installer->newinstall || installer->reinstall) {
//    targv [targc++] = "--newinstall";
//  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  installer->instState = INST_FINISH;
}

/* support routines */

static void
installerCleanup (installer_t *installer)
{
  if (installer->target != NULL) {
    free (installer->target);
  }
}

static void
installerDisplayText (installer_t *installer, char *pfx, char *txt, bool bold)
{
  if (bold) {
    uiTextBoxAppendBoldStr (installer->disptb, pfx);
    uiTextBoxAppendBoldStr (installer->disptb, txt);
    uiTextBoxAppendBoldStr (installer->disptb, "\n");
  } else {
    uiTextBoxAppendStr (installer->disptb, pfx);
    uiTextBoxAppendStr (installer->disptb, txt);
    uiTextBoxAppendStr (installer->disptb, "\n");
  }
  installer->scrolltoend = true;
}

static void
installerTemplateCopy (const char *dir, const char *from, const char *to)
{
  char      *localetmpldir;
  char      tbuff [MAXPATHLEN];

  localetmpldir = sysvarsGetStr (SV_LOCALE);
  snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
      dir, localetmpldir, from);
  logMsg (LOG_INSTALL, LOG_MAIN, "check file: %s", tbuff);
  if (fileopFileExists (tbuff)) {
    logMsg (LOG_INSTALL, LOG_MAIN, "   found");
    from = tbuff;
  } else {
    localetmpldir = sysvarsGetStr (SV_LOCALE_SHORT);
    snprintf (tbuff, sizeof (tbuff), "%s/%s/%s",
        dir, localetmpldir, from);
    logMsg (LOG_INSTALL, LOG_MAIN, "check file: %s", tbuff);
    if (fileopFileExists (tbuff)) {
      logMsg (LOG_INSTALL, LOG_MAIN, "   found");
      from = tbuff;
    } else {
      snprintf (tbuff, sizeof (tbuff), "%s/%s", dir, from);
      from = tbuff;
    }
  }
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "- copy: %s", from);
  logMsg (LOG_INSTALL, LOG_IMPORTANT, "    to: %s", to);
  filemanipBackup (to, 1);
  filemanipCopy (from, to);
}

static void
installerFailWorkingDir (installer_t *installer, const char *dir)
{
  fprintf (stderr, "Unable to set working dir: %s\n", dir);
  /* CONTEXT: installer: failure message */
  installerDisplayText (installer, "", _("Error: Unable to set working folder."), false);
  /* CONTEXT: installer: status message */
  installerDisplayText (installer, " * ", _("Installation aborted."), false);
  installer->instState = INST_WAIT_USER;
}

static void
installerSetTargetDir (installer_t *installer, const char *fn)
{
  if (installer->target != NULL) {
    free (installer->target);
  }
  installer->target = strdup (fn);
  pathNormPath (installer->target, strlen (installer->target));
}

static void
installerCheckAndFixTarget (char *buff, size_t sz)
{
  pathinfo_t  *pi;
  char        *nm;
  int         rc;

  pi = pathInfo (buff);
  nm = BDJ4_NAME;
  if (isMacOS ()) {
    nm = "BDJ4.app";
  }
  rc = strncmp (pi->filename, nm, pi->flen) == 0 &&
      pi->flen == strlen (nm);
  if (! rc) {
    strlcat (buff, "/", sz);
    strlcat (buff, nm, sz);
  }
  pathInfoFree (pi);
}
