/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#import <Foundation/NSLocale.h>
#import <IOKit/pwr_mgt/IOPMLib.h>

#include "bdj4.h"
#include "bdj4intl.h"
#include "mdebug.h"
#include "osutils.h"

static IOPMAssertionID  sleepAssertionID;
static char             *reasonstr = NULL;

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
