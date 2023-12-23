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
#include "ilist.h"
#include "mdebug.h"
#include "pathbld.h"
#include "slist.h"
#include "sysvars.h"

enum {
  MAX_DESC = 10,
};

ilist_t *
dyInterfaceList (const char *pfx, const char *funcnm)
{
  ilist_t     *interfaces;
  ilistidx_t  ikey = 0;
  slist_t     *files;
  slistidx_t  iteridx;
  const char  *fn;
  dlhandle_t  *dlHandle;
  char        dlpath [MAXPATHLEN];
  char        tmp [100];
  void        (*descProc) (dylist_t *, int);
  dylist_t    descarr [MAX_DESC];
  size_t      pfxlen;

  pfxlen = strlen (pfx);

  interfaces = ilistAlloc ("intfc-list", LIST_ORDERED);

  files = dirlistBasicDirList (sysvarsGetStr (SV_BDJ4_DIR_EXEC), sysvarsGetStr (SV_SHLIB_EXT));
  slistStartIterator (files, &iteridx);
  while ((fn = slistIterateKey (files, &iteridx)) != NULL) {
    if (strncmp (fn, pfx, pfxlen) == 0) {
      pathbldMakePath (dlpath, sizeof (dlpath), fn, "", PATHBLD_MP_DIR_EXEC);
      dlHandle = dylibLoad (dlpath);
      if (dlHandle != NULL) {
        descProc = dylibLookup (dlHandle, funcnm);
        if (descProc != NULL) {
          const char  *desc;
          int         c = 0;
          int         idx;

          descProc (descarr, MAX_DESC);
          while ((desc = descarr [c].desc) != NULL) {
            idx = descarr [c].index;
            strlcpy (tmp, fn, sizeof (tmp));
            tmp [strlen (tmp) - strlen (sysvarsGetStr (SV_SHLIB_EXT))] = '\0';
            ilistSetStr (interfaces, ikey, DYI_LIB, tmp);
            ilistSetStr (interfaces, ikey, DYI_DESC, desc);
            ilistSetNum (interfaces, ikey, DYI_INDEX, idx);
            ++c;
            ++ikey;
          }
        }
        /* using the address sanitizer comes up with spurious leaks */
        /* if the dynamic library is closed */
#if ! defined (BDJ4_USING_SANITIZER)
        dylibClose (dlHandle);
#endif
      }
    }
  }
  slistFree (files);

  return interfaces;
}

