/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "roman.h"

/* a simplistic number to roman number converter */
/* this is used for classical music numbers which are not likely */
/* to go over 10 */
int
convertToRoman (int value, char * buff, size_t sz)
{
  size_t    idx;
  int       rc = 0;

  *buff = '\0';
  idx = 0;

  if (value <= 0) {
    return 0;
  }

  while (value >= 10 && idx < sz) {
    buff [idx++] = 'X';
    value -= 10;
  }
  while (value >= 9 && idx < sz) {
    buff [idx++] = 'I';
    buff [idx++] = 'X';
    value -= 9;
  }
  while (value >= 5 && idx < sz) {
    buff [idx++] = 'V';
    value -= 5;
  }
  while (value >= 4 && idx < sz) {
    buff [idx++] = 'I';
    buff [idx++] = 'V';
    value -= 4;
  }
  while (value > 0 && idx < sz) {
    buff [idx++] = 'I';
    value -= 1;
  }

  if (idx < sz) {
    buff [idx++] = '\0';
  } else {
    rc = -1;
  }

  if (value > 0) {
    rc = -1;
  }
  return rc;
}
