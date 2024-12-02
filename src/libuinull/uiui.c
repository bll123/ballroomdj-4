/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
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
#include "uiutils.h"    // for uisetup_t definition

#include "ui/uiwcont-int.h"

#include "ui/uiui.h"

int uiBaseMarginSz = UIUTILS_BASE_MARGIN_SZ;

const char *
uiBackend (void)
{
  return "null";
}

void
uiUIInitialize (int direction)
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
uiSetUICSS (uisetup_t *uisetup)
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

void
uiwcontUIInit (uiwcont_t *uiwidget)
{
  return;
}

void
uiwcontUIInit (uiwcont_t *uiwidget)
{
  return;
}

