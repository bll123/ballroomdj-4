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
    callbackFuncD       doublecbfunc;
    callbackFuncI       intcbfunc;
    callbackFuncII      intintcbfunc;
    callbackFuncS       strcbfunc;
    callbackFuncSS      strstrcbfunc;
    callbackFuncSI      strintcbfunc;
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
callbackInitD (callbackFuncD cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->doublecbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitI (callbackFuncI cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->intcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitII (callbackFuncII cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->intintcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitS (callbackFuncS cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->strcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitSS (callbackFuncSS cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->strstrcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

callback_t *
callbackInitSI (callbackFuncSI cbfunc, void *udata)
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
callbackHandlerD (callback_t *cb, double value)
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
callbackHandlerI (callback_t *cb, int32_t value)
{
  bool  rc = false;

  if (cb == NULL) {
    return 0;
  }
  if (cb->intcbfunc == NULL) {
    return 0;
  }

  rc = cb->intcbfunc (cb->udata, value);
  return rc;
}

bool
callbackHandlerII (callback_t *cb, int a, int b)
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

int32_t
callbackHandlerS (callback_t *cb, const char *str)
{
  int32_t   value;

  if (cb == NULL) {
    return 0;
  }
  if (cb->strcbfunc == NULL) {
    return 0;
  }

  value = cb->strcbfunc (cb->udata, str);
  return value;
}

int32_t
callbackHandlerSS (callback_t *cb, const char *a, const char *b)
{
  int32_t   value;

  if (cb == NULL) {
    return 0;
  }
  if (cb->strstrcbfunc == NULL) {
    return 0;
  }

  value = cb->strstrcbfunc (cb->udata, a, b);
  return true;
}

bool
callbackHandlerSI (callback_t *cb, const char *str, int value)
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

