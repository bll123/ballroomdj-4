/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#include "osutils.h"

void
osSuspendSleep (void)
{
  SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
}

void
osResumeSleep (void)
{
  SetThreadExecutionState (ES_CONTINUOUS);
}
