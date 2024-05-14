/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#if _hdr_stdatomic
# include <stdatomic.h>
#endif
#if defined(__STDC_NO_ATOMICS__)
# define _Atomic(type) type
#endif
#include <string.h>
#include <stdarg.h>

#if __linux__ && _hdr_gio_gio

#include <gio/gio.h>

#include "bdjstring.h"
#include "dbusi.h"
#include "mdebug.h"
#include "tmutil.h"

#define DBUS_DEBUG 1

enum {
  DBUS_STATE_CLOSED,
  DBUS_STATE_WAIT,
  DBUS_STATE_OPEN,
};

enum {
  DBUS_TIMEOUT = 500,
  /* documentation states that the bus id will never be zero */
  DBUS_INVALID_BUS = 0,
};

typedef struct dbus {
  GDBusConnection *dconn;
  GDBusNodeInfo   *idata;
  GVariant        *tvariant;
  GVariant        *data;
  GVariant        *result;
  int             busid;
  _Atomic(int)    state;
  _Atomic(int)    busstate;
} dbus_t;

static void dbusNameAcquired (GDBusConnection *connection, const char *name, gpointer udata);
static void dbusNameLost (GDBusConnection *connection, const char *name, gpointer udata);
static void dbusMethodHandler (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static GVariant * dbusPropertyGetHandler (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GError **error, gpointer udata);
static gboolean dbusPropertySetHandler (GDBusConnection *connection, const char *sender, const char *object_path, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);
# if DBUS_DEBUG
static void dumpResult (const char *tag, GVariant *data);
# endif

static GDBusInterfaceVTable vtable = {
    dbusMethodHandler, dbusPropertyGetHandler, dbusPropertySetHandler, { 0 },
};

dbus_t *
dbusConnInit (void)
{
  dbus_t  *dbus;

  dbus = mdmalloc (sizeof (dbus_t));
  dbus->dconn = NULL;
  dbus->idata = NULL;
  dbus->data = NULL;
  dbus->result = NULL;
  dbus->busid = DBUS_INVALID_BUS;
  dbus->state = DBUS_STATE_WAIT;
  dbus->busstate = DBUS_NAME_CLOSED;

  dbus->dconn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  mdextalloc (dbus->dconn);
  if (dbus->dconn != NULL) {
    dbus->state = DBUS_STATE_OPEN;
  }
  return dbus;
}

void
dbusConnectAcquireName (dbus_t *dbus, const char *instname, const char *intfc)
{
  char    fullinstname [200];

  if (dbus == NULL || instname == NULL || intfc == NULL) {
    return;
  }
fprintf (stderr, "acquire-name: %s %s\n", instname, intfc);

  snprintf (fullinstname, sizeof (fullinstname), "%s.%s", intfc, instname);
fprintf (stderr, "full-inst: %s\n", fullinstname);

  dbus->busstate = DBUS_NAME_WAIT;

  dbus->busid = g_bus_own_name_on_connection (dbus->dconn,
      fullinstname, G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
      dbusNameAcquired, dbusNameLost, dbus, NULL);
}

int
dbusCheckAcquireName (dbus_t *dbus)
{
  return dbus->busstate;
}

void
dbusConnClose (dbus_t *dbus)
{
  if (dbus == NULL) {
    return;
  }

  if (dbus->idata != NULL) {
    g_dbus_node_info_unref (dbus->idata);
  }

  if (dbus->busid != DBUS_INVALID_BUS) {
    g_bus_unown_name (dbus->busid);
  }

  if (dbus->dconn != NULL) {
    mdextfree (dbus->dconn);
    g_object_unref (dbus->dconn);
  }

  /* apparently, the data variant does not need to be unref'd */
  if (dbus->result != NULL) {
    mdextfree (dbus->result);
    g_variant_unref (dbus->result);
  }

  dbus->dconn = NULL;
  dbus->idata = NULL;
  dbus->data = NULL;
  dbus->result = NULL;
  dbus->state = DBUS_STATE_CLOSED;
  dbus->busid = DBUS_INVALID_BUS;
  mdfree (dbus);
}

void
dbusMessageInit (dbus_t *dbus)
{
  dbus->data = g_variant_new_parsed ("()");
  mdextalloc (dbus->data);
}

/* used in the cases where a value is wrapped as a variant (e.g. Rate) */
void *
dbusMessageBuild (const char *sdata, ...)
{
  va_list   args;
  GVariant  *tv;

  va_start (args, sdata);
  tv = g_variant_new_va (sdata, NULL, &args);
  mdextalloc (tv);
  va_end (args);
  return tv;
}

/* used in the cases where a value is wrapped as a variant (e.g. Rate) */
void *
dbusMessageBuildObj (const char *path)
{
  GVariant  *tv;

  tv = g_variant_new_object_path (path);
  mdextalloc (tv);
  return tv;
}

void
dbusMessageSetData (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);
  if (dbus->data != NULL) {
    /* dbusMessageInit() should have been called */
    mdextfree (dbus->data);
    g_variant_unref (dbus->data);
  }
  dbus->data = g_variant_new_va (sdata, NULL, &args);
  mdextalloc (dbus->data);
# if DBUS_DEBUG
  dumpResult ("data-va", dbus->data);
# endif
  va_end (args);
}

/* use a gvariant string to set the data */
void
dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);
  if (dbus->data != NULL) {
    /* dbusMessageInit() should have been called */
    mdextfree (dbus->data);
    g_variant_unref (dbus->data);
  }
  dbus->data = g_variant_new_parsed_va (sdata, &args);
  mdextalloc (dbus->data);
# if DBUS_DEBUG
  dumpResult ("data-parsed", dbus->data);
# endif
  va_end (args);
}

bool
dbusMessage (dbus_t *dbus, const char *bus, const char *objpath,
    const char *intfc, const char *method)
{
  bool    rc;

  if (dbus->result != NULL) {
    mdextfree (dbus->result);
    g_variant_unref (dbus->result);
  }
  dbus->result = NULL;

# if DBUS_DEBUG
  fprintf (stderr, "== %s\n   %s\n   %s\n   %s\n", bus, objpath, intfc, method);
  dumpResult ("data-msg", dbus->data);
# endif
  dbus->result = g_dbus_connection_call_sync (dbus->dconn,
      bus, objpath, intfc, method,
      dbus->data, NULL, G_DBUS_CALL_FLAGS_NONE, DBUS_TIMEOUT, NULL, NULL);
  mdextalloc (dbus->result);
# if DBUS_DEBUG
  dumpResult ("result", dbus->result);
# endif

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
# if DBUS_DEBUG
  fprintf (stderr, "-- result: %s\n", type);
# endif
  if (strcmp (type, "(v)") == 0) {
    g_variant_get (val, type, &val);
    type = g_variant_get_type_string (val);
    /* supported-uri-schemes returns a (v);as */
  }

  if (strcmp (type, "(as)") == 0) {
    GVariantIter  gvi;

    g_variant_iter_init (&gvi, val);
    val = g_variant_iter_next_value (&gvi);
    type = g_variant_get_type_string (val);
    /* list-names returns a (as);as */
  }

  if (strcmp (type, "as") == 0) {
    const char  ***out;
    long        *alen;
    gsize       len;

    out = va_arg (args, const char ***);
    alen = va_arg (args, long *);
    *out = g_variant_get_strv (val, &len);
    mdextalloc (*out);
    *alen = len;
    va_end (args);
    return true;
  }

  if (strcmp (type, "a{sv}") == 0) {
    GVariantIter  gvi;
    GVariant      *ival;
    GVariant      *tv;
    void          *trackid = NULL;
    int64_t       *dur = NULL;
    int           rc = 0;

    g_variant_iter_init (&gvi, val);

    trackid = va_arg (args, void *);
    dur = va_arg (args, int64_t *);

    /* want mpris:trackid, mpris:length */
    while ((ival = g_variant_iter_next_value (&gvi)) != NULL) {
      const char    *idstr;

      mdextalloc (ival);
      type = g_variant_get_type_string (ival);
      g_variant_get (ival, type, &idstr, &tv);
      type = g_variant_get_type_string (tv);
      if (strcmp (idstr, "mpris:trackid") == 0) {
        char    *tstr;

        g_variant_get (tv, type, &tstr);
        mdextalloc (tstr);
        strlcpy (trackid, tstr, DBUS_MAX_TRACKID);
        mdfree (tstr);
        ++rc;
      }
      if (dur != NULL && strcmp (idstr, "mpris:length") == 0) {
        g_variant_get (tv, type, dur);
        ++rc;
      }

      if (rc >= 2 || (rc >= 1 && dur == NULL)) {
        /* only interested in trackid and duration */
        break;
      }
      mdextfree (ival);
      g_variant_unref (ival);
    }
    va_end (args);
    return true;
  }

  g_variant_get_va (val, type, NULL, &args);
  va_end (args);

  return true;
}

void
dbusSetIntrospectionData (dbus_t *dbus, const char *introspection_xml)
{
  GError      *error = NULL;

  if (dbus == NULL) {
    return;
  }

  dbus->idata = g_dbus_node_info_new_for_xml (introspection_xml, &error);
  if (error != NULL) {
    fprintf (stderr, "%s\n", error->message);
  }
}

int
dbusRegisterObject (dbus_t *dbus, const char *objpath, const char *intfc, void *udata)
{
  GDBusInterfaceInfo  *info;
  int                 intfcid;
  GError              *error = NULL;

  if (dbus == NULL || dbus->idata == NULL) {
    return 0;
  }

  info = g_dbus_node_info_lookup_interface (dbus->idata, intfc);

  intfcid = g_dbus_connection_register_object (
      dbus->dconn, objpath, info, &vtable, dbus, NULL, &error);
  if (error != NULL) {
    fprintf (stderr, "%s\n", error->message);
  }

  return intfcid;
}

void
dbusUnregisterObject (dbus_t *dbus, int intfcid)
{
  g_dbus_connection_unregister_object (dbus->dconn, intfcid);
}

/* internal routines */

static void
dbusNameAcquired (GDBusConnection *connection, const char *name, gpointer udata)
{
  dbus_t  *dbus = udata;

  if (dbus == NULL) {
    return;
  }

fprintf (stderr, "  acquired %s\n", name);
  dbus->busstate = DBUS_NAME_OPEN;
}

static void
dbusNameLost (GDBusConnection *connection, const char *name, gpointer udata)
{
  dbus_t  *dbus = udata;

  if (dbus == NULL) {
    return;
  }

fprintf (stderr, "  lost %s\n", name);
  dbus->busstate = DBUS_NAME_CLOSED;
}

static void
dbusMethodHandler (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer udata)
{
  return;
}

static GVariant *
dbusPropertyGetHandler (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GError **error,
    gpointer udata)
{
  return NULL;
}

static gboolean
dbusPropertySetHandler (GDBusConnection *connection,
    const char *sender,
    const char *object_path,
    const char *interface_name,
    const char *property_name,
    GVariant *value,
    GError **error,
    gpointer udata)
{
  return FALSE;
}

# if DBUS_DEBUG

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
      mdextalloc (v);
      dumpResult ("value-as", v);
      mdextfree (v);
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

# endif

#endif  /* __linux__ and _hdr_gio_gio */
