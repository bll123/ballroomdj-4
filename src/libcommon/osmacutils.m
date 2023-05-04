/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#if __APPLE__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#import <IOKit/pwr_mgt/IOPMLib.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "mdebug.h"
#include "osutils.h"

static IOPMAssertionID  sleepAssertionID;
static char             *reasonstr = NULL;

char *
osRegistryGet (char *key, char *name)
{
  return NULL;
}

char *
osGetSystemFont (const char *gsettingspath)
{
  return NULL;
}

void
osSuspendSleep (void)
{
  char          tmp [128];
  CFStringRef   reasonForActivity;
  IOReturn      success;


  /* CONTEXT: MacOS: reason for disabling sleep: BDJ4 Player is Active */
  snprintf (tmp, sizeof (tmp), _("%s Player is Active"), BDJ4_NAME);
  reasonstr = mdstrdup (tmp);
  reasonForActivity = CFStringCreateWithCString (
      kCFAllocatorDefault, reasonstr, kCFStringEncodingUTF8);

  // kIOPMAssertionTypeNoDisplaySleep prevents display sleep.
  // kIOPMAssertionTypeNoIdleSleep prevents idle sleep (display may sleep).
  success = IOPMAssertionCreateWithName (
      kIOPMAssertionTypeNoDisplaySleep,
      kIOPMAssertionLevelOn, reasonForActivity, &sleepAssertionID);
  if (success != kIOReturnSuccess) {
    dataFree (reasonstr);
    reasonstr = NULL;
  }

  return;
}

void
osResumeSleep (void)
{
  if (reasonstr != NULL) {
    IOPMAssertionRelease (sleepAssertionID);
    dataFree (reasonstr);
    reasonstr = NULL;
  }
  return;
}

#endif
