/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bdjstring.h"
#include "fileop.h"
#include "osprocess.h"
#include "osutils.h"

enum {
  BUFFSZ = (10 * 1024 * 1024),
};

#define TAGSTRPFX  "!~~"
#define TAGSTR     "BDJ4"
#define TAGSTRSFX  "~~!"

#define BDJ4_INST_DIR  "bdj4-install"

static char * memsrch (char *buff, size_t bsz, char *srch, size_t ssz);

int
main (int argc, const char *argv [])
{
  struct stat statbuf;
  const char  *fn = argv [0];
  FILE        *ifh = NULL;
  FILE        *archivefh = NULL;
  char        *buff = NULL;
  char        tagstr [40];
  char        tbuff [1024];
  char        tmpdir [1024];
  char        unpackdir [1024];
  char        *archivep = NULL;
  char        *p = NULL;
  char        *tfn = NULL;
  ssize_t     sz;
  bool        first = true;
  int         rc;
  bool        isWindows = false;
  char        *archivenm = "bdj4-install.tar.gz";
  const char  *targv [40];
  int         targc = 0;


#if __WINNT__
  isWindows = true;
  archivenm = "bdj4-install.cab";
# if BDJ4_USE_GTK
  osSetEnv ("GTK_THEME", "Windows-10-Dark");
# endif
#endif

  buff = malloc (BUFFSZ);
  assert (buff != NULL);

  /* a mess to make sure the tag string doesn't appear in the bdj4se binary */
  strlcpy (tagstr, TAGSTRPFX, sizeof (tagstr));
  strlcat (tagstr, TAGSTR, sizeof (tagstr));
  strlcat (tagstr, TAGSTRSFX, sizeof (tagstr));

  ifh = fileopOpen (fn, "rb");
  if (ifh == NULL) {
    fprintf (stderr, "Unable to open input %s %d %s\n", fn, errno, strerror (errno));
    exit (1);
  }

  osGetEnv ("TMPDIR", tmpdir, sizeof (tmpdir));
  if (! *tmpdir) {
    osGetEnv ("TEMP", tmpdir, sizeof (tmpdir));
  }
  if (! *tmpdir) {
    osGetEnv ("TMP", tmpdir, sizeof (tmpdir));
  }
  if (! *tmpdir) {
    rc = stat ("/var/tmp", &statbuf);
    if (rc == 0) {
      strlcpy (tmpdir, "/var/tmp", sizeof (tmpdir));
    }
  }
  if (! *tmpdir) {
    rc = stat ("/tmp", &statbuf);
    if (rc == 0) {
      strlcpy (tmpdir, "/tmp", sizeof (tmpdir));
    }
  }
  if (! *tmpdir) {
#if _args_mkdir == 2 && _define_S_IRWXU
    mkdir ("tmp", S_IRWXU);
#endif
#if _args_mkdir == 1
    mkdir ("tmp");
#endif
    strlcpy (tmpdir, "tmp", sizeof (tmpdir));
  }

  snprintf (tbuff, sizeof (tbuff), "%s/%s", tmpdir, archivenm);
  archivefh = fileopOpen (tbuff, "wb");
  if (archivefh == NULL) {
    fprintf (stderr, "Unable to open output %s %d %s\n", tbuff, errno, strerror (errno));
    exit (1);
  }

  printf ("-- Unpacking installation.\n");
  fflush (stdout);
  sz = fread (buff, 1, BUFFSZ, ifh);
  while (sz > 0) {
    p = buff;

    if (first) {
      archivep = memsrch (p, sz, tagstr, strlen (tagstr));
      if (archivep == NULL) {
        fprintf (stderr, "Unable to locate second tag\n");
        exit (1);
      }

      /* calculate the size of the second block */
      archivep += strlen (tagstr);
      p = archivep;
      sz = sz - (archivep - buff);

      first = false;
    }

    sz = fwrite (p, 1, sz, archivefh);
    sz = fread (buff, 1, BUFFSZ, ifh);
  }

  fclose (ifh);
  fclose (archivefh);

  free (buff);

  rc = chdir (tmpdir);
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", tmpdir, errno, strerror (errno));
    exit (1);
  }

  rc = stat (BDJ4_INST_DIR, &statbuf);
  if (rc == 0) {
    if (isWindows) {
      (void) ! system ("rmdir /s/q bdj4-install > NUL");
    } else {
      (void) ! system ("rm -rf bdj4-install");
    }
  }

  printf ("-- Unpacking archive.\n");
  fflush (stdout);
  if (isWindows) {
    (void) ! system ("expand bdj4-install.cab -F:* . > bdj4-expand.log ");
  } else {
    (void) ! system ("tar -x -f bdj4-install.tar.gz");
  }

  printf ("-- Cleaning temporary files.\n");
  fflush (stdout);

  if (fileopFileExists (archivenm)) {
    fileopDelete (archivenm);
  }
  tfn = "bdj4-expand.log";
  if (fileopFileExists (tfn)) {
    fileopDelete (tfn);
  }

  rc = chdir (BDJ4_INST_DIR);
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", BDJ4_INST_DIR, errno, strerror (errno));
    exit (1);
  }
  /* the unpackdir argument is the very top level, not using Contents/MacOS */
  snprintf (unpackdir, sizeof (unpackdir), "%s/%s", tmpdir, BDJ4_INST_DIR);

  rc = stat ("Contents", &statbuf);
  if (rc == 0) {
    rc = chdir ("Contents/MacOS");
    if (rc != 0) {
      fprintf (stderr, "Unable to chdir to %s %d %s\n", "Contents/MacOS", errno, strerror (errno));
      exit (1);
    }
  }

  printf ("-- Starting install process.\n");
  fflush (stdout);

  if (isWindows) {
    targv [targc++] = ".\\bin\\bdj4.exe";
  } else {
    targv [targc++] = "./bin/bdj4";
  }
  targv [targc++] = "--bdj4installer";
  targv [targc++] = "--unpackdir";
  targv [targc++] = unpackdir;
  for (int i = 1; i < argc && i < 39; ++i) {
    targv [targc++] = argv [i];
  }
  targv [targc++] = NULL;

  osProcessStart (targv, OS_PROC_WAIT, NULL, NULL);
  return 0;
}

static char *
memsrch (char *buff, size_t bsz, char *srch, size_t ssz)
{
  char    *p = NULL;
  char    *sp = NULL;
  size_t  currsz;

  p = buff;
  currsz = bsz;
  while (true) {
    sp = memchr (p, *srch, currsz);

    if (sp == NULL) {
      return NULL;
    }
    if (ssz > (bsz - (sp - buff)) ) {
      return NULL;
    }
    if (memcmp (sp, srch, ssz) == 0) {
      return sp;
    }
    p = sp;
    ++p;
    currsz -= (p - buff);
  }

  return NULL;
}
