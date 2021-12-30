#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "vlci.h"
#include "sysvars.h"
#include "log.h"

int
main (int argc, char *argv[])
{
  vlcData_t *     vlcData;

  sysvarsInit ();
  logStartAppend ("bdj4player", "p");

  vlcData = vlcInit (0, NULL);
  assert (vlcData != NULL);
  vlcClose (vlcData);
  return 0;
}
