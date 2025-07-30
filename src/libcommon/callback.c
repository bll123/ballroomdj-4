/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "bdj4.h"
#include "callback.h"
#include "log.h"
#include "mdebug.h"

enum {
  CB_IDENT_VOID     = 0xaa0064696f764243,
  CB_IDENT_DOUBLE   = 0xbbaa006c62644243,
  CB_IDENT_INT      = 0xbbaa00746e694243,
  CB_IDENT_INT_INT  = 0xccbbaa0069694243,
  CB_IDENT_STR      = 0xbbaa007274734243,
  CB_IDENT_STR_STR  = 0xccbbaa0073734243,
  CB_IDENT_STR_INT  = 0xccbbaa0069734243,
};

typedef struct callback {
  uint64_t        ident;
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

static bool callbackValidate (callback_t *cb, uint64_t wantident);

void
callbackFree (callback_t *cb)
{
  if (cb == NULL) {
    return;
  }

  cb->ident = BDJ4_IDENT_FREE;
  mdfree (cb);
}

[[nodiscard]]
callback_t *
callbackInit (callbackFunc cbfunc, void *udata, const char *actiontext)
{
  callback_t    *cb;


  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_VOID;
  cb->cbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = actiontext;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitD (callbackFuncD cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_DOUBLE;
  cb->doublecbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitI (callbackFuncI cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_INT;
  cb->intcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitII (callbackFuncII cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_INT_INT;
  cb->intintcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitS (callbackFuncS cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_STR;
  cb->strcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitSS (callbackFuncSS cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_STR_STR;
  cb->strstrcbfunc = cbfunc;
  cb->udata = udata;
  cb->actiontext = NULL;
  return cb;
}

[[nodiscard]]
callback_t *
callbackInitSI (callbackFuncSI cbfunc, void *udata)
{
  callback_t    *cb;

  cb = mdmalloc (sizeof (callback_t));
  cb->ident = CB_IDENT_STR_INT;
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

  rc = callbackValidate (cb, CB_IDENT_VOID);
  if (rc == false) {
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

  rc = callbackValidate (cb, CB_IDENT_DOUBLE);
  if (rc == false) {
    return rc;
  }

  rc = cb->doublecbfunc (cb->udata, value);
  return rc;
}


bool
callbackHandlerI (callback_t *cb, int32_t value)
{
  bool  rc = false;

  rc = callbackValidate (cb, CB_IDENT_INT);
  if (rc == false) {
    return rc;
  }

  rc = cb->intcbfunc (cb->udata, value);
  return rc;
}

bool
callbackHandlerII (callback_t *cb, int32_t a, int32_t b)
{
  bool  rc = false;

  rc = callbackValidate (cb, CB_IDENT_INT_INT);
  if (rc == false) {
    return rc;
  }

  rc = cb->intintcbfunc (cb->udata, a, b);
  return rc;
}

int32_t
callbackHandlerS (callback_t *cb, const char *str)
{
  int32_t   value;
  bool      rc = false;

  rc = callbackValidate (cb, CB_IDENT_STR);
  if (rc == false) {
    return 0;
  }

  value = cb->strcbfunc (cb->udata, str);
  return value;
}

int32_t
callbackHandlerSS (callback_t *cb, const char *a, const char *b)
{
  int32_t   value;
  bool      rc = false;

  rc = callbackValidate (cb, CB_IDENT_STR_STR);
  if (rc == false) {
    return 0;
  }

  value = cb->strstrcbfunc (cb->udata, a, b);
  return value;
}

bool
callbackHandlerSI (callback_t *cb, const char *str, int32_t value)
{
  bool  rc = false;

  rc = callbackValidate (cb, CB_IDENT_STR_INT);
  if (rc == false) {
    return rc;
  }

  rc = cb->strintcbfunc (cb->udata, str, value);
  return rc;
}

/* internal routines */

static bool
callbackValidate (callback_t *cb, uint64_t wantident)
{
  bool    rc = false;


  if (cb == NULL) {
    return rc;
  }
  if (cb->cbfunc == NULL) {
    return rc;
  }
  if (cb->ident != wantident) {
    fprintf (stderr, "ERR: callback: mismatch type: %s want: %s\n",
        (char *) &cb->ident, (char *) &wantident);
    return rc;
  }

  rc = true;
  return rc;
}
