/*
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "volume.h"

int
volumeGet (char *sinkname)
{
  int               vol;
  int               rc;

  vol = 0;
  rc = process (VOL_GET, sinkname, &vol, NULL);
  audioDisconnect ();
  return vol;
}

int
volumeSet (char *sinkname, int vol)
{
  int               rc;

  vol = 0;
  rc = process (VOL_SET, sinkname, &vol, NULL);
  audioDisconnect ();
  return vol;
}


int
volumeGetSinkList (char *sinkname, char **sinklist)
{
  int               rc;
  int               vol;

  vol = 0;
  rc = process (VOL_GETSINKLIST, sinkname, &vol, sinklist);
  audioDisconnect ();
  return vol;
}


