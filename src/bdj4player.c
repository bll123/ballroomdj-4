#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "vlci.h"
#include "sysvars.h"

int
main (int argc, char *argv[])
{
  vlcData_t *     vlcData;

  sysvarsInit ();

  vlcData = vlcInit (0, NULL);
  vlcClose (vlcData);
  return 0;
}
