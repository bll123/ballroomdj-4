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

#include "osutils.h"

#define BUFFSZ  (10 * 1024 * 1024)
#define TAGSTRPFX  "!~~"
#define TAGSTR     "BDJ4"
#define TAGSTRSFX  "~~!"

static char * memsrch (char *buff, size_t bsz, char *srch, size_t ssz);

int
main (int argc, char *argv [])
{
  struct stat statbuf;
  char        *fn = argv [0];
  FILE        *ifh = NULL;
  FILE        *zfh = NULL;
  FILE        *mufh = NULL;
  char        *buff = NULL;
  char        tagstr [40];
  char        tbuff [1024];
  char        *tmpdir;
  char        *mup = NULL;
  char        *zp = NULL;
  char        *p = NULL;
  ssize_t     sz;
  bool        first = true;
  int         rc;
  bool        isWindows = false;
  char        *archivenm = "bdj4-install.tar.gz";


#if __WINNT__
  isWindows = true;
  archivenm = "bdj4-install.zip";
  osSetEnv ("GTK_THEME", "Windows-10-Dark");
#endif

  buff = malloc (BUFFSZ);
  assert (buff != NULL);

  /* a mess to make sure the tag string doesn't appear in the bdj4se binary */
  strcpy (tagstr, TAGSTRPFX);
  strcat (tagstr, TAGSTR);
  strcat (tagstr, TAGSTRSFX);

  ifh = fopen (fn, "rb");
  if (ifh == NULL) {
    fprintf (stderr, "Unable to open input %s %d %s\n", fn, errno, strerror (errno));
    exit (1);
  }

  tmpdir = getenv ("TMPDIR");
  if (tmpdir == NULL) {
    tmpdir = getenv ("TEMP");
  }
  if (tmpdir == NULL) {
    tmpdir = getenv ("TMP");
  }
  if (tmpdir == NULL) {
    rc = stat ("/var/tmp", &statbuf);
    if (rc == 0) {
      tmpdir = "/var/tmp";
    }
  }
  if (tmpdir == NULL) {
    rc = stat ("/tmp", &statbuf);
    if (rc == 0) {
      tmpdir = "/tmp";
    }
  }
  if (tmpdir == NULL) {
#if _args_mkdir == 2 && _define_S_IRWXU
    mkdir ("tmp", S_IRWXU);
#endif
#if _args_mkdir == 1
    mkdir ("tmp");
#endif
    tmpdir = "tmp";
  }

  if (isWindows) {
    snprintf (tbuff, sizeof (tbuff), "%s/%s", tmpdir, "miniunz.exe");
    mufh = fopen (tbuff, "wb");
    if (mufh == NULL) {
      fprintf (stderr, "Unable to open output %s %d %s\n", tbuff, errno, strerror (errno));
      exit (1);
    }
  }

  snprintf (tbuff, sizeof (tbuff), "%s/%s", tmpdir, archivenm);
  zfh = fopen (tbuff, "wb");
  if (zfh == NULL) {
    fprintf (stderr, "Unable to open output %s %d %s\n", tbuff, errno, strerror (errno));
    exit (1);
  }

  printf ("-- Unpacking installation.\n");
  fflush (stdout);
  sz = fread (buff, 1, BUFFSZ, ifh);
  while (sz > 0) {
    p = buff;

    if (first) {
      ssize_t   tsz;

      if (isWindows) {
        mup = memsrch (buff, sz, tagstr, strlen (tagstr));
        if (mup == NULL) {
          fprintf (stderr, "Unable to locate first tag\n");
          exit (1);
        }
        mup += strlen (tagstr);
        p = mup;
        ++p;
      } else {
        mup = buff;
      }

      zp = memsrch (p, sz, tagstr, strlen (tagstr));
      if (zp == NULL) {
        fprintf (stderr, "Unable to locate second tag\n");
        exit (1);
      }

      if (isWindows) {
        /* calculate the size of the first block */
        tsz = (zp - mup);
        tsz = fwrite (mup, 1, tsz, mufh);
        fclose (mufh);
      }

      /* calculate the size of the second block */
      zp += strlen (tagstr);
      p = zp;
      sz = sz - (zp - buff);

      first = false;
    }

    sz = fwrite (p, 1, sz, zfh);
    sz = fread (buff, 1, BUFFSZ, ifh);
  }

  fclose (ifh);
  fclose (zfh);

  free (buff);

  rc = chdir (tmpdir);
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", tmpdir, errno, strerror (errno));
    exit (1);
  }

  rc = stat ("bdj4-install", &statbuf);
  if (rc == 0) {
    if (isWindows) {
      system ("rmdir /s/q bdj4-install > NUL");
    } else {
      system ("rm -rf bdj4-install");
    }
  }

  printf ("-- Unpacking archive.\n");
  fflush (stdout);
  if (isWindows) {
    system (".\\miniunz.exe -o -x bdj4-install.zip > bdj4-unzip.log ");
  } else {
    system ("tar -x -f bdj4-install.tar.gz");
  }

  rc = chdir ("bdj4-install");
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", "bdj4-install", errno, strerror (errno));
    exit (1);
  }

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
    argv [0] = ".\\install\\install-startup.bat";
  } else {
    argv [0] = "./install/install-startup.sh";
  }
  osProcessStart (argv, OS_PROC_NONE, NULL, NULL);

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
