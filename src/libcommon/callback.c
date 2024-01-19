/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "callback.h"
#include "log.h"
#include "mdebug.h"

typedef struct callback {
  union {
    callbackFunc        cbfunc;
    callbackFuncDouble  doublecbfunc;
    callbackFuncIntInt  intintcbfunc;
    callbackFuncLong    longcbfunc;
    callbackFuncLongInt longintcbfunc;
    callbackFuncStr     strcbfunc;
    callbackFuncStrInt  strintcbfunc;
  };
  void            *udata;
  const char      *actiontext;
} callback_t;

void
callbackFree (callback_t *cb)
{
  if (cb != NULL) {
    mdfree (cb);
  }
}

callback_t *
callbackInit (callbackFunc cbfunc, void *udata, const char *actiontext)
{
  callback_t    *cb;


  cb = mdmalloc (sizeof (callback_t));
  cb->cbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = actiontext;
  return cb;
}

callback_t *
callbackInitDouble (callbackFuncDouble cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->doublecbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitIntInt (callbackFuncIntInt cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->intintcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitLong (callbackFuncLong cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->longcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitLongInt (callbackFuncLongInt cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->longintcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitStr (callbackFuncStr cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->strcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitStrInt (callbackFuncStrInt cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->strintcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

bool
callbackIsSet (callback_t *cb)
{
  if (cb == NULL) {
    return false;
  }
  if (cb->cbfunc == NULL) {
    return false;
  }

  return true;
}

bool
callbackHandler (callback_t *cb)
{
  bool  rc = false;

  if (cb == NULL) {
    return rc;
  }
  if (cb->cbfunc == NULL) {
    return rc;
  }

  if (cb->actiontext != NULL) {
    logMsg (LOG_DBG, LOG_ACTIONS, "= action: %s", cb->actiontext);
  }
  rc = cb->cbfunc (cb->udata);
  return rc;
}

bool
callbackHandlerDouble (callback_t *cb, double value)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->doublecbfunc == NULL) {
    return 0;
  }

  rc = cb->doublecbfunc (cb->udata, value);
  return rc;
}


bool
callbackHandlerLong (callback_t *cb, long value)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->longcbfunc == NULL) {
    return 0;
  }

  rc = cb->longcbfunc (cb->udata, value);
  return rc;
}

bool
callbackHandlerIntInt (callback_t *cb, int a, int b)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->intintcbfunc == NULL) {
    return 0;
  }

  rc = cb->intintcbfunc (cb->udata, a, b);
  return rc;
}

bool
callbackHandlerLongInt (callback_t *cb, long lval, int ival)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->longintcbfunc == NULL) {
    return 0;
  }

  rc = cb->longintcbfunc (cb->udata, lval, ival);
  return rc;
}

long
callbackHandlerStr (callback_t *cb, const char *str)
{
  long    value;

  if (cb == NULL) {
    return 0;
  }
  if (cb->strcbfunc == NULL) {
    return 0;
  }

  value = cb->strcbfunc (cb->udata, str);
  return value;
}

bool
callbackHandlerStrInt (callback_t *cb, const char *str, int value)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->strcbfunc == NULL) {
    return 0;
  }

  rc = cb->strintcbfunc (cb->udata, str, value);
  return rc;
}

