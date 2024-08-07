/*
 * Copyright 2023-2024 Brad Lanam Pleasant Hill CA
 *
 * This could be re-written to use libdbus.
 * The glib variants are a pain to use.
 * Is libdbus any easier?
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
  GVariantBuilder gvbuild;
  int             acount;
  int             busid;
  _Atomic(int)    state;
  _Atomic(int)    busstate;
  dbusCBmethod_t  cbmethod;
  dbusCBpropget_t cbpropget;
  void            *userdata;
} dbus_t;

static void dbusFreeData (dbus_t *dbus);
static void dbusNameAcquired (GDBusConnection *connection, const char *name, gpointer udata);
static void dbusNameLost (GDBusConnection *connection, const char *name, gpointer udata);
static void dbusMethodHandler (GDBusConnection *connection, const char *sender, const char *objpath, const char *interface_name, const char *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer udata);
static GVariant * dbusPropertyGetHandler (GDBusConnection *connection, const char *sender, const char *objpath, const char *interface_name, const char *property_name, GError **error, gpointer udata);
static gboolean dbusPropertySetHandler (GDBusConnection *connection, const char *sender, const char *objpath, const char *interface_name, const char *property_name, GVariant *value, GError **error, gpointer udata);
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
  dbus->cbmethod = NULL;
  dbus->userdata = NULL;

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

  if (dbus->data != NULL) {
    mdextfree (dbus->data);
// fprintf (stderr, "data-unref: %d\n", G_IS_OBJECT (dbus->data));
//    if (G_IS_OBJECT (dbus->data)) {
//      g_variant_unref (dbus->data);
//    }
  }

  if (dbus->result != NULL) {
    mdextfree (dbus->result);
//fprintf (stderr, "result-unref: %d\n", G_IS_OBJECT (dbus->result));
//    if (G_IS_OBJECT (dbus->result)) {
      g_variant_unref (dbus->result);
//    }
  }

  if (dbus->idata != NULL) {
    g_dbus_node_info_unref (dbus->idata);
  }

  if (dbus->busid != DBUS_INVALID_BUS) {
    g_bus_unown_name (dbus->busid);
  }

  if (dbus->dconn != NULL) {
    mdextfree (dbus->dconn);
fprintf (stderr, "dconn-unref: %d\n", G_IS_OBJECT (dbus->dconn));
    g_object_unref (dbus->dconn);
  }

  /* apparently, the data variant does not need to be unref'd */

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
  if (dbus == NULL) {
    return;
  }

  dbus->data = g_variant_new_parsed ("()");
}

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

void *
dbusMessageBuildObj (const char *objpath)
{
  GVariant  *tv;

  tv = g_variant_new_object_path (objpath);
  mdextalloc (tv);
  return tv;
}

void
dbusMessageSetData (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);

  dbusFreeData (dbus);

  dbus->data = g_variant_new_va (sdata, NULL, &args);
# if DBUS_DEBUG
  dumpResult ("data-va", dbus->data);
# endif
  va_end (args);
}

/* want to get the types from the children, i don't know another way */
/* how to create an 'a{sv}' given the variant container */
void
dbusMessageSetDataTuple (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;
  GVariant  **children;
  void      *v;
  int       count;
  int       i;

  count = 0;
  va_start (args, sdata);
  while ((v = va_arg (args, void *)) != NULL) {
    ++count;
  }
  va_end (args);

  dbusFreeData (dbus);

  children = mdmalloc (sizeof (GVariant *) * count);

  i = 0;
  va_start (args, sdata);
  while ((v = va_arg (args, void *)) != NULL) {
    children [i] = v;
    ++i;
  }
  va_end (args);

  dbus->data = g_variant_new_tuple (children, count);
  mdfree (children);
# if DBUS_DEBUG
  dumpResult ("data-va", dbus->data);
# endif
}

void
dbusMessageSetDataArray (dbus_t *dbus, const char *sdata, ...)
{
  va_list         args;
  GVariant        *v;
  GVariant        *tv;
  const char      *str;

  va_start (args, sdata);
  dbusFreeData (dbus);
  dbus->acount = 0;
  g_variant_builder_init (&dbus->gvbuild, G_VARIANT_TYPE (sdata));

  if (strcmp (sdata, "as") == 0) {
    while ((str = va_arg (args, const char *)) != NULL) {
      ++dbus->acount;
      g_variant_builder_add (&dbus->gvbuild, "s", str);
    }
  }
  if (strcmp (sdata, "a{sv}") == 0) {
    str = va_arg (args, const char *);
    v = va_arg (args, void *);
    ++dbus->acount;
    g_variant_builder_add (&dbus->gvbuild, "{sv}", str, v);
  }
  tv = g_variant_builder_end (&dbus->gvbuild);
  dbus->data = tv;
  mdextalloc (dbus->data);
# if DBUS_DEBUG
  dumpResult ("data-va-a", dbus->data);
# endif
  va_end (args);
}

void *
dbusMessageEmptyArray (const char *sdata)
{
  GVariant          *ta;

  /* builder cannot build an empty array! */
  ta = g_variant_new_array (G_VARIANT_TYPE (sdata + 1), NULL, 0);
  return ta;
}

void
dbusMessageInitArray (dbus_t *dbus, const char *sdata)
{
  g_variant_builder_init (&dbus->gvbuild, G_VARIANT_TYPE (sdata));
  dbus->acount = 0;
}

void
dbusMessageAppendArray (dbus_t *dbus, const char *sdata, ...)
{
  va_list         args;
  GVariant        *v;
  const char      *str;

  va_start (args, sdata);
  dbusFreeData (dbus);

  if (strcmp (sdata, "as") == 0) {
    str = va_arg (args, const char *);
    g_variant_builder_add (&dbus->gvbuild, "s", str);
    ++dbus->acount;
  }
  if (strcmp (sdata, "a{sv}") == 0) {
    str = va_arg (args, const char *);
    v = va_arg (args, void *);
# if DBUS_DEBUG
  dumpResult ("aa-v", v);
# endif
    g_variant_builder_add (&dbus->gvbuild, "{sv}", str, v);
    ++dbus->acount;
  }
  va_end (args);
}

void *
dbusMessageFinalizeArray (dbus_t *dbus)
{
  void      *rv;
  GVariant  *tv;

  if (dbus->acount > 0) {
    tv = g_variant_builder_end (&dbus->gvbuild);
# if DBUS_DEBUG
  dumpResult ("fin-arr", tv);
# endif
    rv = tv;
  } else {
    rv = &dbus->gvbuild;
  }
  return rv;
}

/* use a gvariant string to set the data */
/* used by dbustest */
void
dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...)
{
  va_list   args;

  va_start (args, sdata);

  dbusFreeData (dbus);

  dbus->data = g_variant_new_parsed_va (sdata, &args);
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
fprintf (stderr, "msg:result-unref: %d\n", G_IS_OBJECT (dbus->result));
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
  const char  *type = NULL;
  GVariant    *val = NULL;
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
  if (type != NULL && strcmp (type, "(v)") == 0) {
    g_variant_get (val, type, &val);
    type = g_variant_get_type_string (val);
    /* supported-uri-schemes returns a (v);as */
  }

  if (type != NULL && strcmp (type, "(as)") == 0) {
    GVariantIter  gvi;

    g_variant_iter_init (&gvi, val);
    val = g_variant_iter_next_value (&gvi);
    type = g_variant_get_type_string (val);
    /* list-names returns a (as);as */
  }

  if (type != NULL && strcmp (type, "as") == 0) {
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

  if (type != NULL && strcmp (type, "a{sv}") == 0) {
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
fprintf (stderr, "ival-unref: %d\n", G_IS_OBJECT (ival));
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
dbusRegisterObject (dbus_t *dbus, const char *objpath, const char *intfc)
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
  if (dbus == NULL || dbus->dconn == NULL) {
    return;
  }
  g_dbus_connection_unregister_object (dbus->dconn, intfcid);
}

void
dbusSetCallbacks (dbus_t *dbus, void *udata, dbusCBmethod_t cbmethod,
    dbusCBpropget_t cbpropget)
{
  if (dbus == NULL) {
    return;
  }

  dbus->cbmethod = cbmethod;
  dbus->cbpropget = cbpropget;
  dbus->userdata = udata;
}

void
dbusEmitSignal (dbus_t *dbus, const char *bus, const char *objpath,
    const char *intfc, const char *property)
{
  GError              *gerror = NULL;

# if DBUS_DEBUG
  fprintf (stderr, "== %s\n   %s\n   %s\n", objpath, intfc, property);
  dumpResult ("emit", dbus->data);
# endif
  g_dbus_connection_emit_signal (dbus->dconn, bus,
      objpath, intfc, property, dbus->data, &gerror);
  if (gerror != NULL) {
    fprintf (stderr, "%s\n", gerror->message);
  }
}

/* internal routines */

static void
dbusFreeData (dbus_t *dbus)
{
  if (dbus == NULL || dbus->data == NULL) {
    return;
  }

  mdextfree (dbus->data);
fprintf (stderr, "fd-data-unref: %d\n", G_IS_OBJECT (dbus->data));
  if (G_IS_OBJECT (dbus->data)) {
    g_variant_unref (dbus->data);
  }
}

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
    const char *objpath,
    const char *intfc,
    const char *method,
    GVariant *parameters,
    GDBusMethodInvocation *invocation,
    gpointer udata)
{
  dbus_t    *dbus = udata;

fprintf (stderr, "dbus-method: %s %s %s\n", objpath, intfc, method);
# if DBUS_DEBUG
  dumpResult ("  method-params", parameters);
# endif

  if (dbus->cbmethod != NULL) {
    dbus->cbmethod (intfc, method, dbus->userdata);
  }

  g_dbus_method_invocation_return_value (invocation, NULL);
  return;
}

static GVariant *
dbusPropertyGetHandler (GDBusConnection *connection,
    const char *sender,
    const char *objpath,
    const char *intfc,
    const char *property,
    GError **error,
    gpointer udata)
{
  dbus_t    *dbus = udata;

fprintf (stderr, "dbus-prop-get: %s %s %s\n", objpath, intfc, property);

  if (dbus->cbpropget != NULL) {
    if (dbus->cbpropget (intfc, property, dbus->userdata) == false) {
      g_set_error (error, G_DBUS_ERROR,
          G_DBUS_ERROR_UNKNOWN_PROPERTY,
          "Unknown property %s", property);
    }
  }

  return dbus->data;
}

static gboolean
dbusPropertySetHandler (GDBusConnection *connection,
    const char *sender,
    const char *objpath,
    const char *intfc,
    const char *property,
    GVariant *value,
    GError **error,
    gpointer udata)
{
fprintf (stderr, "dbus-prop-set: %s %s %s\n", objpath, intfc, property);
  return TRUE;
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

#if 0
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
#endif
  {
    char    *ts;

    ts = g_variant_print (data, true);
    fprintf (stderr, "  data: %s\n", ts);
    free (ts);
  }
  fflush (stderr);
}

# endif

#endif  /* __linux__ and _hdr_gio_gio */
