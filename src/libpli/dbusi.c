/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#if __linux__ && _hdr_gio_gio

#include <gio/gio.h>

#include "dbus.h"
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
  if (dbus->data != NULL) {
    g_variant_unref (dbus->data);
  }
  if (dbus->result != NULL) {
    g_variant_unref (dbus->result);
  }
  dbus->dconn = NULL;
  dbus->dconn = NULL;
  dbus->state = DBUS_STATE_CLOSED;
}

void
dbusMessageInit (dbus_t *dbus)
{
  if (dbus->data != NULL) {
    g_variant_unref (dbus->data);
  }
  dbus->data = g_variant_new_parsed ("()");
}

void
dbusMessageSetData (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);
  dbus->data = g_variant_new_parsed_va (sdata, &args);
  dumpResult ("data-a", dbus->data);
  va_end (args);
}

void
dbusMessage (dbus_t *dbus, const char *bus, const char *objpath,
    const char *intfc, const char *method)
{
  fprintf (stdout, "== %s\n   %s\n   %s\n   %s\n", bus, objpath, intfc, method);
  dumpResult ("data-b", dbus->data);
  dbus->result = g_dbus_connection_call_sync (dbus->dconn,
      bus, objpath, intfc, method,
      dbus->data, NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_TIMEOUT, NULL, NULL);
  dumpResult ("result", dbus->result);
}

static void
dumpResult (const char *tag, GVariant *data)
{
  const char *type;

  fprintf (stdout, "-- %s\n", tag);
  if (data == NULL) {
    fprintf (stdout, "null\n");
    return;
  }

  type = g_variant_get_type_string (data);
  fprintf (stdout, "  type: %s\n", type);

  if (strcmp (type, "(v)") == 0) {
    GVariant  *v;

    g_variant_get (data, type, &v);
    dumpResult ("value", v);
  } else {
    fprintf (stdout, "  data: %s\n", g_variant_print (data, true));
  }
}

#endif  /* __linux__ and _hdr_gio_gio */
