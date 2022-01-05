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
  rc = volumeProcess (VOL_GET, sinkname, &vol, NULL);
  volumeDisconnect ();
  return vol;
}

int
volumeSet (char *sinkname, int vol)
{
  int               rc;

  vol = 0;
  rc = volumeProcess (VOL_SET, sinkname, &vol, NULL);
  volumeDisconnect ();
  return vol;
}


int
volumeGetSinkList (char *sinkname, char **sinklist)
{
  int               rc;
  int               vol;

  vol = 0;
  rc = volumeProcess (VOL_GETSINKLIST, sinkname, &vol, sinklist);
  volumeDisconnect ();
  return vol;
}


