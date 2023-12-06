/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#pragma clang diagnostic push
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

#include <check.h>

#include "check_bdj.h"
#include "mdebug.h"
#include "log.h"

void
check_libaudiosrc (SRunner *sr)
{
  Suite   *s;

  logMsg (LOG_DBG, LOG_IMPORTANT, "==chk== libaudiosrc");

  s = audiosrc_suite();
  srunner_add_suite (sr, s);
}

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
