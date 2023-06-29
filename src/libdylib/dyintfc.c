#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "bdj4.h"
#include "bdjstring.h"
#include "dirlist.h"
#include "dyintfc.h"
#include "dylib.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

#if 0
typedef struct dyiter {
  char        *pfx;
  size_t      pfxlen;
  char        *funcnm;
  slist_t     *files;
  slistidx_t  iteridx;
  dlhandle_t  *dlHandle;
} dyiter_t;
#endif

slist_t *
dyInterfaceList (const char *pfx, const char *funcnm)
{
  slist_t     *interfaces;
  slist_t     *files;
  slistidx_t  iteridx;
  const char  *fn;
  dlhandle_t  *dlHandle;
  char        dlpath [MAXPATHLEN];
  char        tmp [100];
  const char  *(*descProc) (void);
  const char  *desc;
  size_t      pfxlen;

  pfxlen = strlen (pfx);

  interfaces = slistAlloc ("intfc-list", LIST_UNORDERED, NULL);
  files = dirlistBasicDirList (sysvarsGetStr (SV_BDJ4_DIR_EXEC), sysvarsGetStr (SV_SHLIB_EXT));
  slistStartIterator (files, &iteridx);
  while ((fn = slistIterateKey (files, &iteridx)) != NULL) {
    if (strncmp (fn, pfx, pfxlen) == 0) {
      pathbldMakePath (dlpath, sizeof (dlpath), fn, "", PATHBLD_MP_DIR_EXEC);
      dlHandle = dylibLoad (dlpath);
      if (dlHandle != NULL) {
        descProc = dylibLookup (dlHandle, funcnm);
        if (descProc != NULL) {
          desc = descProc ();
          if (desc != NULL) {
            strlcpy (tmp, fn, sizeof (tmp));
            tmp [strlen (tmp) - strlen (sysvarsGetStr (SV_SHLIB_EXT))] = '\0';
            slistSetStr (interfaces, desc, tmp);
          }
        }
        dylibClose (dlHandle);
      }
    }
  }
  slistFree (files);
  slistSort (interfaces);

  return interfaces;
}

#if 0


dyiter_t *
dyInterfaceStartIterator (const char *pfx, const char *funcnm)
{
  dyiter_t  *dyiter;

  if (pfx == NULL || funcnm == NULL) {
    return NULL;
  }

  dyiter = mdmalloc (sizeof (dyiter_t));
  dyiter->pfx = mdstrdup (pfx);
  dyiter->pfxlen = strlen (pfx);
  dyiter->funcnm = mdstrdup (funcnm);
  dyiter->files = dirlistBasicDirList (sysvarsGetStr (SV_BDJ4_DIR_EXEC), sysvarsGetStr (SV_SHLIB_EXT));
  dyiter->dlHandle = NULL;
  slistStartIterator (dyiter->files, &dyiter->iteridx);
}

void *
dyInterfaceIterate (dyiter_t *dyiter)
{
  const char  *fn;
  char        dlpath [MAXPATHLEN];
  void        *proc;

  if (dyiter == NULL) {
    return NULL;
  }

  if (dyiter->dlHandle != NULL) {
    dylibClose (dyiter->dlHandle);
    dyiter->dlHandle = NULL;
  }

  while ((fn = slistIterateKey (dyiter->files, &dyiter->iteridx)) != NULL) {
    if (strncmp (fn, dyiter->pfx, dyiter->pfxlen) == 0) {
      pathbldMakePath (dlpath, sizeof (dlpath), fn, "", PATHBLD_MP_DIR_EXEC);
      dyiter->dlHandle = dylibLoad (dlpath);
      if (dyiter->dlHandle != NULL) {
        proc = dylibLookup (dyiter->dlHandle, dyiter->funcnm);
        if (proc != NULL) {
          return proc;
        }
      } /* dylib open ok */
    } /* proper prefix */
  } /* for each file */

  return NULL;
}

void
dyInterfaceCleanIterator (dyiter_t *dyiter)
{
  if (dyiter != NULL) {
    dataFree (dyiter->pfx);
    dyiter->pfx = NULL;
    dataFree (dyiter->funcnm);
    slistFree (dyiter->files);
    if (dyiter->dlHandle != NULL) {
      dylibClose (dyiter->dlHandle);
      dyiter->dlHandle = NULL;
    }
    mdfree (dyiter);
  }
}

#endif
