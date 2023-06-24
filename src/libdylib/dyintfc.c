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
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

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

  interfaces = slistAlloc ("intfc-list", LIST_UNORDERED, NULL);
  files = dirlistBasicDirList (sysvarsGetStr (SV_BDJ4_DIR_EXEC), sysvarsGetStr (SV_SHLIB_EXT));
  slistStartIterator (files, &iteridx);
  while ((fn = slistIterateKey (files, &iteridx)) != NULL) {
    if (strncmp (fn, pfx, strlen (pfx)) == 0) {
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
