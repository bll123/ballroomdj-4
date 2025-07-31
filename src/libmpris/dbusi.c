/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 *
 * This could be re-written to use libdbus.
 * The glib variants are a pain to use.
 * Is libdbus any easier?
 */
#if __linux__ && __has_include (<gio/gio.h>)

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <stdarg.h>

#include <gio/gio.h>

#include "bdjstring.h"
#include "dbusi.h"
#include "mdebug.h"
#include "tmutil.h"

#define DBUS_DEBUG 0

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
  GVariant        *tvariant;
  GVariant        *data;
  GVariant        *result;
  GVariantBuilder gvbuild;
  int             acount;
  int             busid;
  _Atomic(int)    state;
  _Atomic(int)    busstate;
  void            *userdata;
  bool            dataalloc;
} dbus_t;

static void dbusFreeData (dbus_t *dbus);
static void dbusNameAcquired (GDBusConnection *connection, const char *name, gpointer udata);
static void dbusNameLost (GDBusConnection *connection, const char *name, gpointer udata);
# if DBUS_DEBUG
static void dumpResult (const char *tag, GVariant *data);
# endif

dbus_t *
dbusConnInit (void)
{
  dbus_t  *dbus;

  dbus = mdmalloc (sizeof (dbus_t));
  dbus->dconn = NULL;
  dbus->data = NULL;
  dbus->dataalloc = false;
  dbus->result = NULL;
  dbus->busid = DBUS_INVALID_BUS;
  dbus->state = DBUS_STATE_WAIT;
  dbus->busstate = DBUS_NAME_CLOSED;
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

  snprintf (fullinstname, sizeof (fullinstname), "%s.%s", intfc, instname);

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

  dbusFreeData (dbus);

  if (dbus->result != NULL) {
    mdextfree (dbus->result);
    g_variant_unref (dbus->result);
  }

  if (dbus->busid != DBUS_INVALID_BUS) {
    g_bus_unown_name (dbus->busid);
  }

  if (dbus->dconn != NULL) {
    mdextfree (dbus->dconn);
    g_object_unref (dbus->dconn);
  }

  dbus->dconn = NULL;
  dbus->data = NULL;
  dbus->dataalloc = false;
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
  dbus->dataalloc = false;
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
  if (strcmp (sdata, "()") != 0) {
    mdextalloc (dbus->data);
    dbus->dataalloc = true;
  }
# if DBUS_DEBUG
  dumpResult ("data-va", dbus->data);
# endif
  va_end (args);
}

#if 0 /* keep this for now */

/* want to get the types from the children, i don't know another way */
/* how to create an 'a{sv}' given the variant container */
void
dbusMessageSetDataTuple (dbus_t *dbus, const char *sdata, ...)  /* KEEP */
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
  mdextalloc (dbus->data);
  dbus->dataalloc = true;
  mdfree (children);
# if DBUS_DEBUG
  dumpResult ("data-va", dbus->data);
# endif
}
#endif

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

  if (strcmp (sdata, "s") == 0) {
    str = va_arg (args, const char *);
    g_variant_builder_add (&dbus->gvbuild, sdata, str);
    ++dbus->acount;
  }
  if (strcmp (sdata, "{sv}") == 0) {
    str = va_arg (args, const char *);
    v = va_arg (args, void *);
# if DBUS_DEBUG
    fprintf (stderr, "aa-v: %s\n", str);
    dumpResult ("aa-v", v);
# endif
    g_variant_builder_add (&dbus->gvbuild, sdata, str, v);
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
  mdextalloc (dbus->data);
  dbus->dataalloc = true;
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
        stpecpy (trackid, trackid + DBUS_MAX_TRACKID, tstr);
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
  dbusFreeData (dbus);

  return true;
}

void
dbusSetInterfaceSkeleton (dbus_t *dbus, void *skel, const char *objpath)
{
  GError  *gerror = NULL;

  g_dbus_interface_skeleton_export (
      (GDBusInterfaceSkeleton *) skel, dbus->dconn, objpath, &gerror);
  if (gerror != NULL) {
    fprintf (stderr, "ERR: %s\n", gerror->message);
  }
}

/* internal routines */

static void
dbusFreeData (dbus_t *dbus)
{
  if (dbus == NULL || dbus->data == NULL) {
    return;
  }

  if (dbus->dataalloc) {
    mdextfree (dbus->data);
    g_variant_unref (dbus->data);
  }
  dbus->data = NULL;
  dbus->dataalloc = false;
}

static void
dbusNameAcquired (GDBusConnection *connection, const char *name, gpointer udata)
{
  dbus_t  *dbus = udata;

  if (dbus == NULL) {
    return;
  }

  dbus->busstate = DBUS_NAME_OPEN;
}

static void
dbusNameLost (GDBusConnection *connection, const char *name, gpointer udata)
{
  dbus_t  *dbus = udata;

  if (dbus == NULL) {
    return;
  }

  dbus->busstate = DBUS_NAME_CLOSED;
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
      /* this seems to crash */
      // g_variant_unref (v);
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
    if (g_variant_is_container (data)) {
      const char  *ts;
      ts = g_variant_get_data (data);
      fprintf (stderr, "  data-c: %s\n", ts);
    }
    if (! g_variant_is_container (data)) {
      char    *ts;

      ts = g_variant_print (data, true);
      fprintf (stderr, "  data: %s\n", ts);
      free (ts);
    }
  }
  fflush (stderr);
}

# endif

#endif  /* __linux__ and gio/gio.h */
