/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "uiclass.h"

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"

int uiBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;

const char *
uiBackend (void)
{
  return "null";
}

void
uiUIInitialize (void)
{
  return;
}

void
uiUIProcessEvents (void)
{
  return;
}

void
uiUIProcessWaitEvents (void)
{
  return;
}

void
uiCleanup (void)
{
  return;
}

void
uiSetUICSS (const char *uifont, const char *accentColor,
    const char *errorColor)
{
  return;
}

void
uiAddColorClass (const char *classnm, const char *color)
{
  return;
}

void
uiAddBGColorClass (const char *classnm, const char *color)
{
  return;
}

void
uiAddProgressbarClass (const char *classnm, const char *color)
{
  return;
}

void
uiInitUILog (void)
{
  return;
}
