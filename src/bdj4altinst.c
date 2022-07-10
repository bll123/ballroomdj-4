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
#include "pathbld.h"
#include "pathutil.h"
#include "slist.h"
#include "sysvars.h"
#include "tmutil.h"
#include "ui.h"

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
  char            *maindir;
  char            *hostname;
  char            *home;
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
} altinst_t;

#define INST_HL_COLOR "#b16400"
#define INST_SEP_COLOR "#733000"

static void altinstBuildUI (altinst_t *altinst);
static int  altinstMainLoop (void *udata);
static bool altinstExitCallback (void *udata);
static bool altinstCheckDirTarget (void *udata);
static bool altinstTargetDirDialog (void *udata);
static int  altinstValidateTarget (uientry_t *entry, void *udata);
static bool altinstInstallCallback (void *udata);
static void altinstSetPaths (altinst_t *altinst);

static void altinstInstInit (altinst_t *altinst);
static void altinstMakeTarget (altinst_t *altinst);
static void altinstChangeDir (altinst_t *altinst);
static void altinstCreateDirs (altinst_t *altinst);
static void altinstCopyTemplates (altinst_t *altinst);
static void altinstCreateShortcut (altinst_t *altinst);
static void altinstUpdateProcessInit (altinst_t *altinst);
static void altinstUpdateProcess (altinst_t *altinst);

static void altinstCleanup (altinst_t *altinst);
static void altinstDisplayText (altinst_t *altinst, char *pfx, char *txt, bool bold);
static void altinstTemplateCopy (const char *dir, const char *from, const char *to);
static void altinstFailWorkingDir (altinst_t *altinst, const char *dir);
static void altinstSetTargetDir (altinst_t *altinst, const char *fn);

int
main (int argc, char *argv[])
{
  altinst_t   altinst;
  char          buff [MAXPATHLEN];
  char          *uifont;

  buff [0] = '\0';

  uiutilsUIWidgetInit (&altinst.window);
  altinst.instState = INST_INITIALIZE;
  altinst.target = strdup ("");
  altinst.uiBuilt = false;
  altinst.scrolltoend = false;
  altinst.maindir = NULL;
  altinst.home = NULL;
  altinst.hostname = NULL;
  uiutilsUIWidgetInit (&altinst.reinstWidget);
  uiutilsUIWidgetInit (&altinst.feedbackMsg);

  altinst.targetEntry = uiEntryInit (80, MAXPATHLEN);

  sysvarsInit (argv[0]);
  localeInit ();

  altinst.maindir = sysvarsGetStr (SV_BDJ4MAINDIR);
  altinst.home = sysvarsGetStr (SV_HOME);
  altinst.hostname = sysvarsGetStr (SV_HOSTNAME);

  uiUIInitialize ();

  uifont = sysvarsGetStr (SV_FONT_DEFAULT);
  if (uifont == NULL || ! *uifont) {
    uifont = "Arial 12";
  }
  uiSetUIFont (uifont);

  altinstBuildUI (&altinst);

  while (altinst.instState != INST_EXIT) {
    altinstMainLoop (&altinst);
    mssleep (10);
  }

  /* process any final events */
  uiUIProcessEvents ();

  altinstCleanup (&altinst);
  logEnd ();
  return 0;
}

static void
altinstBuildUI (altinst_t *altinst)
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
  /* CONTEXT: alt-install: window title */
  snprintf (tbuff, sizeof (tbuff), _("%s Alternate Installer"), BDJ4_NAME);
  uiutilsUICallbackInit (&altinst->callbacks [INST_CALLBACK_EXIT],
      altinstExitCallback, altinst);
  uiCreateMainWindow (&altinst->window,
      &altinst->callbacks [INST_CALLBACK_EXIT],
      tbuff, imgbuff);
  uiWindowSetDefaultSize (&altinst->window, 1000, 600);

  uiCreateVertBox (&vbox);
  uiWidgetSetAllMargins (&vbox, 10);
  uiWidgetSetMarginTop (&vbox, 20);
  uiWidgetExpandHoriz (&vbox);
  uiWidgetExpandVert (&vbox);
  uiBoxPackInWindow (&altinst->window, &vbox);

  uiCreateLabel (&uiwidget,
      /* CONTEXT: alt-install: ask for alternate installation folder */
      _("Enter the alternate folder."));
  uiBoxPackStart (&vbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiEntryCreate (altinst->targetEntry);
  uiEntrySetValue (altinst->targetEntry, altinst->target);
  uiwidgetp = uiEntryGetUIWidget (altinst->targetEntry);
  uiWidgetAlignHorizFill (uiwidgetp);
  uiWidgetExpandHoriz (uiwidgetp);
  uiBoxPackStartExpand (&hbox, uiwidgetp);
  uiEntrySetValidate (altinst->targetEntry,
      altinstValidateTarget, altinst, UIENTRY_DELAYED);

  uiutilsUICallbackInit (&altinst->callbacks [INST_CALLBACK_TARGET_DIR],
      altinstTargetDirDialog, altinst);
  uiCreateButton (&uiwidget,
      &altinst->callbacks [INST_CALLBACK_TARGET_DIR],
      "", NULL);
  uiButtonSetImageIcon (&uiwidget, "folder");
  uiWidgetSetMarginStart (&uiwidget, 0);
  uiBoxPackStart (&hbox, &uiwidget);

  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  /* CONTEXT: alt-install: checkbox: re-install BDJ4 */
  uiCreateCheckButton (&altinst->reinstWidget, _("Re-Install"),
      altinst->reinstall);
  uiBoxPackStart (&hbox, &altinst->reinstWidget);
  uiutilsUICallbackInit (&altinst->reinstcb, altinstCheckDirTarget, altinst);
  uiToggleButtonSetCallback (&altinst->reinstWidget, &altinst->reinstcb);

  uiCreateLabel (&altinst->feedbackMsg, "");
  uiLabelSetColor (&altinst->feedbackMsg, INST_HL_COLOR);
  uiBoxPackStart (&hbox, &altinst->feedbackMsg);

  uiCreateHorizSeparator (&uiwidget);
  uiSeparatorSetColor (&uiwidget, INST_SEP_COLOR);
  uiBoxPackStart (&vbox, &uiwidget);

  /* button box */
  uiCreateHorizBox (&hbox);
  uiWidgetExpandHoriz (&hbox);
  uiBoxPackStart (&vbox, &hbox);

  uiCreateButton (&uiwidget,
      &altinst->callbacks [INST_CALLBACK_EXIT],
      /* CONTEXT: exits the altinst */
      _("Exit"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  uiutilsUICallbackInit (&altinst->callbacks [INST_CALLBACK_INSTALL],
      altinstInstallCallback, altinst);
  uiCreateButton (&uiwidget,
      &altinst->callbacks [INST_CALLBACK_INSTALL],
      /* CONTEXT: alt-install: start the installation process */
      _("Install"), NULL);
  uiBoxPackEnd (&hbox, &uiwidget);

  altinst->disptb = uiTextBoxCreate (200);
  uiTextBoxSetReadonly (altinst->disptb);
  uiTextBoxHorizExpand (altinst->disptb);
  uiTextBoxVertExpand (altinst->disptb);
  uiBoxPackStartExpand (&vbox, uiTextBoxGetScrolledWindow (altinst->disptb));

  uiWidgetShowAll (&altinst->window);
  altinst->uiBuilt = true;
}

static int
altinstMainLoop (void *udata)
{
  altinst_t *altinst = udata;

  uiUIProcessEvents ();

  uiEntryValidate (altinst->targetEntry, false);

  if (altinst->scrolltoend) {
    uiTextBoxScrollToEnd (altinst->disptb);
    altinst->scrolltoend = false;
    uiUIProcessEvents ();
    mssleep (10);
    uiUIProcessEvents ();
    /* go through the main loop once more */
    return TRUE;
  }

  switch (altinst->instState) {
    case INST_INITIALIZE: {
      altinst->instState = INST_PREPARE;
      break;
    }
    case INST_PREPARE: {
      uiEntryValidate (altinst->targetEntry, true);
      altinst->instState = INST_WAIT_USER;
      break;
    }
    case INST_WAIT_USER: {
      /* do nothing */
      break;
    }
    case INST_INIT: {
      altinstInstInit (altinst);
      break;
    }
    case INST_MAKE_TARGET: {
      altinstMakeTarget (altinst);
      break;
    }
    case INST_CHDIR: {
      altinstChangeDir (altinst);
      break;
    }
    case INST_CREATE_DIRS: {
      altinstCreateDirs (altinst);

      logStart ("bdj4altinst", "ai",
          LOG_IMPORTANT | LOG_BASIC | LOG_MAIN | LOG_REDIR_INST);
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "=== alt inst started");
      logMsg (LOG_INSTALL, LOG_IMPORTANT, "target: %s", altinst->target);
      break;
    }
    case INST_COPY_TEMPLATES: {
      altinstCopyTemplates (altinst);
      break;
    }
    case INST_CREATE_SHORTCUT: {
      altinstCreateShortcut (altinst);
      break;
    }
    case INST_UPDATE_PROCESS_INIT: {
      altinstUpdateProcessInit (altinst);
      break;
    }
    case INST_UPDATE_PROCESS: {
      altinstUpdateProcess (altinst);
      break;
    }
    case INST_FINISH: {
      /* CONTEXT: alt-install: status message */
      altinstDisplayText (altinst, "## ",  _("Installation complete."), true);
      altinst->instState = INST_PREPARE;
      break;
    }
    case INST_EXIT: {
      break;
    }
  }

  return TRUE;
}

static bool
altinstCheckDirTarget (void *udata)
{
  altinst_t   *altinst = udata;

  uiEntryValidate (altinst->targetEntry, true);
  return UICB_CONT;
}

static int
altinstValidateTarget (uientry_t *entry, void *udata)
{
  altinst_t   *altinst = udata;
  const char    *dir;
  bool          exists = false;
  bool          tbool;
  char          tbuff [MAXPATHLEN];
  int           rc = UIENTRY_OK;

  if (! altinst->uiBuilt) {
    return UIENTRY_RESET;
  }

  dir = uiEntryGetValue (altinst->targetEntry);
  tbool = uiToggleButtonIsActive (&altinst->reinstWidget);
  altinst->reinstall = tbool;

  exists = fileopIsDirectory (dir);

  if (exists) {
    /* CONTEXT: alt-install: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("Updating existing alternate installation."));
    uiLabelSetText (&altinst->feedbackMsg, tbuff);
  } else {
    /* CONTEXT: alt-install: message indicating the action that will be taken */
    snprintf (tbuff, sizeof (tbuff), _("New alternate installation."));
    uiLabelSetText (&altinst->feedbackMsg, tbuff);
  }

  if (rc == UIENTRY_OK) {
    altinstSetPaths (altinst);
  }
  return rc;
}

static bool
altinstTargetDirDialog (void *udata)
{
  altinst_t *altinst = udata;
  char        *fn = NULL;
  uiselect_t  *selectdata;

  selectdata = uiDialogCreateSelect (&altinst->window,
      /* CONTEXT: alt-install: dialog title for selecting install location */
      _("Install Location"),
      uiEntryGetValue (altinst->targetEntry), NULL, NULL, NULL);
  fn = uiSelectDirDialog (selectdata);
  if (fn != NULL) {
    char        tbuff [MAXPATHLEN];

    strlcpy (tbuff, fn, sizeof (tbuff));
    /* validation gets called again upon set */
    uiEntrySetValue (altinst->targetEntry, tbuff);
    free (fn);
    logMsg (LOG_INSTALL, LOG_IMPORTANT, "selected target loc: %s", altinst->target);
  }
  free (selectdata);
  return UICB_CONT;
}

static bool
altinstExitCallback (void *udata)
{
  altinst_t   *altinst = udata;

  altinst->instState = INST_EXIT;
  return UICB_CONT;
}

static bool
altinstInstallCallback (void *udata)
{
  altinst_t *altinst = udata;

  if (altinst->instState == INST_WAIT_USER) {
    altinst->instState = INST_INIT;
  }
  return UICB_CONT;
}

static void
altinstSetPaths (altinst_t *altinst)
{
  altinstSetTargetDir (altinst, uiEntryGetValue (altinst->targetEntry));
}

/* installation routines */

static void
altinstInstInit (altinst_t *altinst)
{
  altinstSetPaths (altinst);
  altinst->reinstall = uiToggleButtonIsActive (&altinst->reinstWidget);
  altinst->instState = INST_MAKE_TARGET;
}

static void
altinstMakeTarget (altinst_t *altinst)
{
  fileopMakeDir (altinst->target);

  altinst->instState = INST_CHDIR;
}

static void
altinstChangeDir (altinst_t *altinst)
{
  if (chdir (altinst->target)) {
    altinstFailWorkingDir (altinst, altinst->target);
    return;
  }

  altinst->instState = INST_CREATE_DIRS;
}

static void
altinstCreateDirs (altinst_t *altinst)
{
  /* CONTEXT: alt-install: status message */
  altinstDisplayText (altinst, "-- ", _("Creating folder structure."), false);

  /* create the directories that are not included in the distribution */
  fileopMakeDir ("data");
  /* this will create the directories necessary for the configs */
  bdjoptCreateDirectories ();
  fileopMakeDir ("tmp");
  fileopMakeDir ("http");

  altinst->instState = INST_COPY_TEMPLATES;
}

static void
altinstCopyTemplates (altinst_t *altinst)
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


  /* CONTEXT: alt-install: status message */
  altinstDisplayText (altinst, "-- ", _("Copying template files."), false);

  if (chdir (altinst->target)) {
    altinstFailWorkingDir (altinst, altinst->target);
    return;
  }

  renamelist = NULL;

  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-sr", BDJ4_CONFIG_EXT, PATHBLD_MP_INSTDIR);
  srdf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-auto", BDJ4_CONFIG_EXT, PATHBLD_MP_INSTDIR);
  autodf = datafileAllocParse ("loc-sr", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);
  pathbldMakePath (tbuff, sizeof (tbuff),
      "localized-qd", BDJ4_CONFIG_EXT, PATHBLD_MP_INSTDIR);
  qddf = datafileAllocParse ("loc-qd", DFTYPE_KEY_VAL,
      tbuff, NULL, 0, DATAFILE_NO_LOOKUP);

  pathbldMakePath (dir, sizeof (dir),
      "", "", PATHBLD_MP_TEMPLATEDIR);
  dirlist = diropBasicDirList (dir, NULL);
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

    strlcpy (from, fname, sizeof (from));

    if (strcmp (fname, "bdj-flex-dark.html") == 0) {
      snprintf (to, sizeof (to), "http/bdj4remote.html");
      altinstTemplateCopy (dir, from, to);
      continue;
    }
    if (strcmp (fname, "mobilemq.html") == 0) {
      snprintf (to, sizeof (to), "http/%s", fname);
      altinstTemplateCopy (dir, from, to);
      continue;
    }

    pi = pathInfo (fname);
    if (pathInfoExtCheck (pi, ".html")) {
      free (pi);
      continue;
    }

    if (pathInfoExtCheck (pi, ".crt")) {
      snprintf (to, sizeof (to), "http/%s", fname);
    } else if (pathInfoExtCheck (pi, ".svg")) {
      snprintf (to, sizeof (to), "%s/img/%s",
          altinst->target, fname);
    } else if (strncmp (fname, "bdjconfig", 9) == 0) {
      snprintf (tbuff, sizeof (tbuff), "%.*s", (int) pi->blen, pi->basename);
      if (pathInfoExtCheck (pi, ".g")) {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".p")) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else if (pathInfoExtCheck (pi, ".m")) {
        snprintf (to, sizeof (to), "data/%s/%s", altinst->hostname, tbuff);
      } else if (pathInfoExtCheck (pi, ".mp")) {
        snprintf (to, sizeof (to), "data/%s/profile00/%s",
            altinst->hostname, tbuff);
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

      strlcpy (from, tbuff, sizeof (from));
      if (strncmp (pi->basename, "ds-", 3) == 0) {
        snprintf (to, sizeof (to), "data/profile00/%s", tbuff);
      } else {
        snprintf (to, sizeof (to), "data/%s", tbuff);
      }
    } else {
      /* unknown extension */
      free (pi);
      continue;
    }

    altinstTemplateCopy (dir, from, to);

    free (pi);
  }
  slistFree (dirlist);

  snprintf (dir, sizeof (dir), "%s/img", altinst->maindir);

  strlcpy (from, "favicon.ico", sizeof (from));
  snprintf (to, sizeof (to), "http/favicon.ico");
  altinstTemplateCopy (dir, from, to);

  strlcpy (from, "led_on.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_on.svg");
  altinstTemplateCopy (dir, from, to);

  strlcpy (from, "led_off.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/led_off.svg");
  altinstTemplateCopy (dir, from, to);

  strlcpy (from, "ballroomdj4.svg", sizeof (from));
  snprintf (to, sizeof (to), "http/ballroomdj4.svg");
  altinstTemplateCopy (dir, from, to);

  snprintf (from, sizeof (from), "%s/img/mrc", altinst->maindir);
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

  datafileFree (srdf);
  datafileFree (autodf);
  datafileFree (qddf);

  altinst->instState = INST_FINISH;
//  altinst->instState = INST_CREATE_SHORTCUT;
}

static void
altinstCreateShortcut (altinst_t *altinst)
{
  char buff [MAXPATHLEN];

  if (chdir (altinst->maindir)) {
    altinstFailWorkingDir (altinst, altinst->maindir);
    return;
  }

  /* CONTEXT: alt-install: status message */
  altinstDisplayText (altinst, "-- ", _("Creating shortcut."), false);
  if (isWindows ()) {
    if (! chdir ("install")) {
      snprintf (buff, sizeof (buff), ".\\makeshortcut.bat \"%s\"",
          altinst->maindir);
      system (buff);
      chdir (altinst->maindir);
    }
  }
  if (isMacOS ()) {
#if _lib_symlink
    /* on macos, the startup program must be a gui program, otherwise */
    /* the dock icon is not correct */
    /* this must exist and match the name of the app */
    symlink ("bin/bdj4g", "BDJ4");
    /* desktop shortcut */
    snprintf (buff, sizeof (buff), "%s/Desktop/BDJ4.app", altinst->home);
    symlink (altinst->target, buff);
#endif
  }
  if (isLinux ()) {
    snprintf (buff, sizeof (buff), "./install/linuxshortcut.sh '%s'",
        altinst->maindir);
    system (buff);
  }

  altinst->instState = INST_UPDATE_PROCESS;
}

static void
altinstUpdateProcessInit (altinst_t *altinst)
{
  char  buff [MAXPATHLEN];

  /* CONTEXT: alt-install: status message */
  snprintf (buff, sizeof (buff), _("Updating %s."), BDJ4_LONG_NAME);
  altinstDisplayText (altinst, "-- ", buff, false);
  altinst->instState = INST_UPDATE_PROCESS;
}

static void
altinstUpdateProcess (altinst_t *altinst)
{
  char  tbuff [MAXPATHLEN];
  int   targc = 0;
  const char  *targv [10];

  snprintf (tbuff, sizeof (tbuff), "%s/bin/bdj4%s",
      altinst->maindir, sysvarsGetStr (SV_OS_EXEC_EXT));
  targv [targc++] = tbuff;
  targv [targc++] = "--bdj4updater";

  /* only need to run the 'newinstall' update process when the template */
  /* files have been copied */
// ### fix
//  if (altinst->newinstall || altinst->reinstall) {
//    targv [targc++] = "--newinstall";
//  }
  targv [targc++] = NULL;
  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);

  altinst->instState = INST_FINISH;
}

/* support routines */

static void
altinstCleanup (altinst_t *altinst)
{
  if (altinst->target != NULL) {
    free (altinst->target);
  }
}

static void
altinstDisplayText (altinst_t *altinst, char *pfx, char *txt, bool bold)
{
  if (bold) {
    uiTextBoxAppendBoldStr (altinst->disptb, pfx);
    uiTextBoxAppendBoldStr (altinst->disptb, txt);
    uiTextBoxAppendBoldStr (altinst->disptb, "\n");
  } else {
    uiTextBoxAppendStr (altinst->disptb, pfx);
    uiTextBoxAppendStr (altinst->disptb, txt);
    uiTextBoxAppendStr (altinst->disptb, "\n");
  }
  altinst->scrolltoend = true;
}

static void
altinstTemplateCopy (const char *dir, const char *from, const char *to)
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
altinstFailWorkingDir (altinst_t *altinst, const char *dir)
{
  fprintf (stderr, "Unable to set working dir: %s\n", dir);
  /* CONTEXT: alt-install: failure message */
  altinstDisplayText (altinst, "", _("Error: Unable to set working folder."), false);
  /* CONTEXT: alt-install: status message */
  altinstDisplayText (altinst, " * ", _("Installation aborted."), false);
  altinst->instState = INST_WAIT_USER;
}

static void
altinstSetTargetDir (altinst_t *altinst, const char *fn)
{
  if (altinst->target != NULL) {
    free (altinst->target);
  }
  altinst->target = strdup (fn);
  pathNormPath (altinst->target, strlen (altinst->target));
}

