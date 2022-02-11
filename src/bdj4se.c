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
  FILE        *ifh;
  FILE        *zfh;
  FILE        *mufh;
  char        *buff;
  char        tagstr [40];
  char        tbuff [1024];
  char        *tmpdir;
  char        *mup;
  char        *zp;
  char        *p;
  ssize_t     sz;
  bool        first = true;
  int         rc;


  buff = malloc (BUFFSZ);
  assert (buff != NULL);

  /* a mess to make sure the tag string doesn't appear in the bdj4se binary */
  strcpy (tagstr, TAGSTRPFX);
  strcat (tagstr, TAGSTR);
  strcat (tagstr, TAGSTRSFX);

  ifh = fopen (fn, "rb");
  if (ifh == NULL) {
    fprintf (stderr, "Unable to open input %d %s\n", errno, strerror (errno));
    exit (1);
  }

  tmpdir = getenv ("TEMP");
  if (tmpdir == NULL) {
#if _args_mkdir == 2 && _define_S_IRWXU
    mkdir ("tmp", S_IRWXU);
#endif
#if _args_mkdir == 1
    mkdir ("tmp");
#endif
    tmpdir = "tmp";
  }

  snprintf (tbuff, sizeof (tbuff), "%s/%s", tmpdir, "miniunz.exe");
  mufh = fopen (tbuff, "wb");
  if (mufh == NULL) {
    fprintf (stderr, "Unable to open output %d %s\n", errno, strerror (errno));
    exit (1);
  }

  snprintf (tbuff, sizeof (tbuff), "%s/%s", tmpdir, "bdj4-install.zip");
  zfh = fopen (tbuff, "wb");
  if (zfh == NULL) {
    fprintf (stderr, "Unable to open output %d %s\n", errno, strerror (errno));
    exit (1);
  }

  printf ("-- Unpacking installation.\n");
  sz = fread (buff, 1, BUFFSZ, ifh);
  while (sz > 0) {
    p = buff;

    if (first) {
      ssize_t   tsz;

      mup = memsrch (buff, sz, tagstr, strlen (tagstr));
      if (mup == NULL) {
        fprintf (stderr, "Unable to locate first tag\n");
        exit (1);
      }
      mup += strlen (tagstr);
      p = mup;
      ++p;
      zp = memsrch (p, sz, tagstr, strlen (tagstr));
      if (zp == NULL) {
        fprintf (stderr, "Unable to locate second tag\n");
        exit (1);
      }

      /* calculate the size of the first block */
      tsz = (zp - mup);
      tsz = fwrite (mup, 1, tsz, mufh);

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
  fclose (mufh);
  fclose (zfh);

  free (buff);

  rc = chdir (tmpdir);
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", tmpdir, errno, strerror (errno));
    exit (1);
  }

  rc = stat ("bdj4-install", &statbuf);
  if (rc == 0) {
#if _lib_CreateFile  /* check if it is windows */
    system ("rmdir /s/q bdj4-install > NUL");
#endif
  }

  printf ("-- Unpacking archive.\n");
  system (".\\miniunz.exe -o -x bdj4-install.zip > bdj4-unzip.log ");

  rc = chdir ("bdj4-install");
  if (rc != 0) {
    fprintf (stderr, "Unable to chdir to %s %d %s\n", "bdj4-install", errno, strerror (errno));
    exit (1);
  }

  printf ("-- Starting install process.\n");
  system ("start cmd.exe /c .\\install\\install-prefix.bat");

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
