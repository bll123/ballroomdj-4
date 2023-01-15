#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "conn.h"
#include "m3u.h"
#include "mp3exp.h"
#include "musicdb.h"
#include "nlist.h"
#include "pathutil.h"

void
mp3Export (char *msg, musicdb_t *musicdb, const char *dirname, int mqidx)
{
  char        tname [200];
  char        tslname [200];
  nlist_t     *tlist = NULL;
  pathinfo_t  *pi;

  pi = pathInfo (dirname);
  snprintf (tname, sizeof (tname), "%s/%.*s.m3u", dirname,
      (int) pi->flen, pi->filename);
  snprintf (tslname, sizeof (tslname), "%.*s",
      (int) pi->flen, pi->filename);
  pathInfoFree (pi);

//  savenames = aaExportMP3 (mqidx, dirname);
  m3uExport (musicdb, tlist, tname, tslname);
}

