/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#if __linux__ && _hdr_gio_gio

#include <gio/gio.h>

#include "dbusi.h"
#include "mdebug.h"

enum {
  DBUS_STATE_CLOSED,
  DBUS_STATE_WAIT,
  DBUS_STATE_OPEN,
};

enum {
  DBUS_TIMEOUT = 500,
};

typedef struct dbus {
  GDBusConnection *dconn;
  GVariant        *data;
  GVariant        *result;
  int             state;
} dbus_t;

static void dumpResult (const char *tag, GVariant *data);

dbus_t *
dbusConnInit (void)
{
  dbus_t  *dbus;

  dbus = mdmalloc (sizeof (dbus_t));
  dbus->dconn = NULL;
  dbus->data = NULL;
  dbus->result = NULL;
  dbus->state = DBUS_STATE_WAIT;

  dbus->dconn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (dbus->dconn != NULL) {
    dbus->state = DBUS_STATE_OPEN;
  }
  return dbus;
}

void
dbusConnClose (dbus_t *dbus)
{
  g_object_unref (dbus->dconn);
  /* apparently, the data variant does not need to be unref'd */
  if (dbus->result != NULL) {
    g_variant_unref (dbus->result);
  }
  dbus->dconn = NULL;
  dbus->data = NULL;
  dbus->result = NULL;
  dbus->state = DBUS_STATE_CLOSED;
}

void
dbusMessageInit (dbus_t *dbus)
{
  dbus->data = g_variant_new_parsed ("()");
}

void
dbusMessageSetData (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);
  dbus->data = g_variant_new_va (sdata, NULL, &args);
  dumpResult ("data-va", dbus->data);
  va_end (args);
}

/* use a gvariant string to set the data */
void
dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);
  dbus->data = g_variant_new_parsed_va (sdata, &args);
  dumpResult ("data-parsed", dbus->data);
  va_end (args);
}

bool
dbusMessage (dbus_t *dbus, const char *bus, const char *objpath,
    const char *intfc, const char *method)
{
  bool    rc;

  if (dbus->result != NULL) {
    g_variant_unref (dbus->result);
  }
  dbus->result = NULL;

  fprintf (stderr, "== %s\n   %s\n   %s\n   %s\n", bus, objpath, intfc, method);
  dumpResult ("data-msg", dbus->data);
  dbus->result = g_dbus_connection_call_sync (dbus->dconn,
      bus, objpath, intfc, method,
      dbus->data, NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_TIMEOUT, NULL, NULL);
  dumpResult ("result", dbus->result);

  rc = dbus->result == NULL ? false : true;
  return rc;
}

bool
dbusResultGet (dbus_t *dbus, ...)
{
  const char  *type;
  GVariant    *val;
  va_list     args;

  val = dbus->result;
  if (val == NULL) {
    return false;
  }

  va_start (args, dbus);

  type = g_variant_get_type_string (val);
fprintf (stderr, "-- result: %s\n", type);
  if (strcmp (type, "(v)") == 0) {
    g_variant_get (val, type, &val);
    type = g_variant_get_type_string (val);
  }
  if (strcmp (type, "(as)") == 0) {
    GVariantIter  gvi;
    gsize         len;
    const char    ***out;
    long          *alen;

    g_variant_iter_init (&gvi, val);
    val = g_variant_iter_next_value (&gvi);
    type = g_variant_get_type_string (val);
    out = va_arg (args, const char ***);
    alen = va_arg (args, long *);
    *out = g_variant_get_strv (val, &len);
    fprintf (stderr, "  g-as-count: %ld\n", len);
    *alen = len;
    va_end (args);
    return true;
  }

  g_variant_get_va (val, type, NULL, &args);
  va_end (args);

  return true;
}

/* internal routines */

static void
dumpResult (const char *tag, GVariant *data)
{
  const char  *type;
  GVariant    *v;


  fprintf (stderr, "-- %s\n", tag);
  if (data == NULL) {
    fprintf (stderr, "null\n");
    return;
  }

  type = g_variant_get_type_string (data);
  fprintf (stderr, "  type: %s\n", type);

  if (strcmp (type, "(v)") == 0) {
    g_variant_get (data, type, &v);
    dumpResult ("value", v);
  } else if (strcmp (type, "(as)") == 0) {
    GVariantIter  gvi;

    g_variant_iter_init (&gvi, data);
    fprintf (stderr, "  as-count: %lu\n", g_variant_iter_n_children (&gvi));

    while ((v = g_variant_iter_next_value (&gvi)) != NULL) {
      dumpResult ("value-as", v);
      g_variant_unref (v);
    }
  } else if (strcmp (type, "as") == 0) {
    const char  **out;
    gsize       len;

    out = g_variant_get_strv (data, &len);
    for (gsize i = 0; i < len; ++i) {
      const char    *tstr;

      tstr = out [i];
      fprintf (stderr, "  array: %s\n", tstr);
    }
  } else {
    fprintf (stderr, "  data: %s\n", g_variant_print (data, true));
  }
}

#endif  /* __linux__ and _hdr_gio_gio */
