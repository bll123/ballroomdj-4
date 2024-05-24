#include "config.h"

#if _hdr_pipewire_pipewire

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>

/*
 *
 * https://gitlab.freedesktop.org/pipewire/pipewire/-/wikis/Migrate-PulseAudio
 *
 * pw-cli
 * https://gitlab.freedesktop.org/pipewire/pipewire/-/blob/master/src/tools/pw-cli.c?ref_type=heads
 *   list-objects Node
 *     media.class == Audio/Sink  done
 *   info <node-id>
 *      device.id             done
 *      card.profile.device   done
 *   enum-params <node-id> Format
 *      can check for stereo output here
 *   enum-params <device-id> Route
 *      indicates which route is in use     done
 *
 *  set-volume for the route
 *   pw-cli set-param <device-id> \
 *       Route '{ index: <route-index>, device: <card-profile-device>, \
 *       props: { mute: false, channelVolumes: [ 0.5, 0.5 ] }, save: true }'
 *
 * pw-metadata shows the defaults
 *
 *  Defaults
 *    pw-metadata
 *
 */

#include <pipewire/pipewire.h>
#include <pipewire/keys.h>
#include <pipewire/extensions/metadata.h>
#include <spa/utils/keys.h>
#include <spa/utils/json.h>
#include <spa/param/param.h>
#include <spa/param/param-types.h>
#include <spa/pod/parser.h>
#include <spa/debug/pod.h>

#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

enum {
  PW_TYPE_DEVICE,
  PW_TYPE_NODE,
};

typedef struct pwproxy pwproxy_t;

typedef struct {
  struct pw_main_loop   *pwmainloop;
  struct pw_context     *context;
  struct pw_core        *core;
  struct pw_registry    *registry;
  struct spa_hook       registry_listener;
  struct pw_metadata    *metadata;
  struct spa_hook       metadata_listener;
  struct pw_properties  *props;
  struct pw_map         pwdevlist;
  struct pw_map         pwsinklist;
  volsinklist_t         *sinklist;
  char                  *defname;
  uint32_t              metadata_id;
  int                   pwsinkcount;
  int                   coreseq;
} pwstate_t;

typedef struct {
  int                   internalid;
  uint32_t              nodeid;
  uint32_t              deviceid;
  uint32_t              cardprofiledev;
  int                   routeidx;
  char                  *description;
  char                  *name;
  struct pw_node_info   *info;
  pwproxy_t             *deviceProxy;
} pwsink_t;

typedef struct pwproxy {
  int                   type;
  uint32_t              deviceid;
  pwstate_t             *pwstate;
  pwsink_t              *pwsink;
  struct spa_hook       object_listener;
//  struct spa_hook       proxy_listener;
  struct pw_proxy       *proxy;
} pwproxy_t;

static pwstate_t *pipewireInit (void);
static void pipewireRegistryEvent (void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props);
static int pipewireGetSink (void *item, void *udata);
static void pipewireCheckMainLoop (void *udata, uint32_t id, int seq);
static void pipewireRunOnce (pwstate_t *pwstate);
static int pipewireMetadataEvent (void *udata, uint32_t id, const char *key, const char *type, const char *value);
static void pipewireNodeInfoEvent (void *udata, const struct pw_node_info *info);
static void pipewireParamEvent (void *udata, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param);
static int pipewireGetRoute (void *item, void *udata);
static int pipewireMapDevice (void *item, void *udata);

static const struct pw_registry_events registry_events = {
  PW_VERSION_REGISTRY_EVENTS,
  .global = pipewireRegistryEvent,
  .global_remove = NULL,
// ### write global remove ?
};

static const struct pw_metadata_events metadata_events = {
  PW_VERSION_METADATA_EVENTS,
  .property = pipewireMetadataEvent,
};

static const struct pw_node_events node_events = {
  PW_VERSION_NODE_EVENTS,
  .info = pipewireNodeInfoEvent,
  .param = pipewireParamEvent,
};

static const struct pw_device_events device_events = {
    PW_VERSION_DEVICE_EVENTS,
    .param = pipewireParamEvent,
};

void
voliDesc (const char **ret, int max)
{
  int   c = 0;

  if (max < 2) {
    return;
  }

  ret [c++] = "PipeWire";
  ret [c++] = NULL;
}

void
voliDisconnect (void)
{
  return;
}

void
voliCleanup (void **udata)
{
  pwstate_t   *pwstate = *udata;

  if (pwstate != NULL) {
    if (pwstate->registry != NULL) {
      mdextfree (pwstate->registry);
      pw_proxy_destroy ((struct pw_proxy *) pwstate->registry);
      pwstate->registry = NULL;
    }

    if (pwstate->metadata != NULL) {
      mdextfree (pwstate->metadata);
      pw_proxy_destroy ((struct pw_proxy *) pwstate->metadata);
      pwstate->metadata = NULL;
    }

// pw_map_for_each
// pw_map_clear

    if (pwstate->core != NULL) {
      mdextfree (pwstate->core);
      pw_core_disconnect (pwstate->core);
      pwstate->core = NULL;
    }

    if (pwstate->context != NULL) {
      mdextfree (pwstate->context);
      pw_context_destroy (pwstate->context);
      pwstate->context = NULL;
    }

    if (pwstate->pwmainloop != NULL) {
      mdextfree (pwstate->pwmainloop);
      pw_main_loop_destroy (pwstate->pwmainloop);
      pwstate->pwmainloop = NULL;
    }

    dataFree (pwstate->defname);
    pwstate->defname = NULL;

    pw_deinit ();
    mdfree (pwstate);
    *udata = NULL;
  }

  return;
}

int
voliProcess (volaction_t action, const char *sinkname,
    int *vol, volsinklist_t *sinklist, void **udata)
{
  pwstate_t   *pwstate = *udata;

  if (action == VOL_HAVE_SINK_LIST) {
    return true;
  }

  if (pwstate == NULL) {
    pwstate = pipewireInit ();

    /* run once to get the nodes */
    pipewireRunOnce (pwstate);
    /* run again to get the metadata and node-info */
    pipewireRunOnce (pwstate);
  }

  /* process any outstanding events */
  pipewireRunOnce (pwstate);

  if (action == VOL_GETSINKLIST) {
    pwstate->sinklist = sinklist;
    dataFree (sinklist->defname);
    sinklist->defname = NULL;
    sinklist->sinklist = NULL;
    sinklist->count = 0;
    sinklist->sinklist = mdrealloc (sinklist->sinklist,
        pwstate->pwsinkcount * sizeof (volsinkitem_t));

    pw_map_for_each (&pwstate->pwsinklist, pipewireGetSink, pwstate);

    return 0;
  }

  if (action == VOL_SET) {
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    return *vol;
  }

  return 0;
}

/* internal routines */

static pwstate_t *
pipewireInit (void)
{
  pwstate_t *pwstate = NULL;

  pw_init (0, NULL);

  pwstate = mdmalloc (sizeof (pwstate_t));
  pwstate->pwmainloop = NULL;
  pwstate->props = NULL;
  pwstate->context = NULL;
  pwstate->core = NULL;
  pwstate->registry = NULL;
  pwstate->metadata = NULL;
  pwstate->defname = NULL;
  pwstate->pwsinkcount = 0;

  pwstate->pwmainloop = pw_main_loop_new (NULL);
  mdextalloc (pwstate->pamainloop);
//    pw_loop_add_signal (pw_main_loop_get_loop (pwstate->pwmainloop),
//        SIGINT, pipewireSigHandler, pwstate);
//    pw_loop_add_signal (pw_main_loop_get_loop (pwstate->pwmainloop),
//        SIGTERM, pipewireSigHandler, pwstate);

  pwstate->props = pw_properties_new (PW_KEY_MEDIA_TYPE, "Audio",
      PW_KEY_MEDIA_CATEGORY, "Playback",
      PW_KEY_MEDIA_ROLE, "Music",
      NULL);
  mdextalloc (pwstate->props);

  pwstate->context = pw_context_new (
      pw_main_loop_get_loop (pwstate->pwmainloop),
      pwstate->props, 0);
  mdextalloc (pwstate->context);

  pwstate->core = pw_context_connect (pwstate->context, pwstate->props, 0);
  mdextalloc (pwstate->core);

  pw_map_init (&pwstate->pwdevlist, 8, 1);
  pw_map_init (&pwstate->pwsinklist, 4, 1);

  pwstate->registry = pw_core_get_registry (pwstate->core, PW_VERSION_REGISTRY, 0);
  mdextalloc (pwstate->registry);

  pw_registry_add_listener (pwstate->registry,
      &pwstate->registry_listener, &registry_events, pwstate);

  return pwstate;
}

static void
pipewireRegistryEvent (void *data, uint32_t id,
    uint32_t permissions, const char *type, uint32_t version,
    const struct spa_dict *props)
{
  pwstate_t                   *pwstate = data;
  const struct spa_dict_item  *item;
  pwsink_t                    *sink;
  struct pw_proxy             *proxy;
  pwproxy_t                   *pd;

  if (props == NULL || props->n_items == 0) {
    return;
  }

  if (strcmp (type, PW_TYPE_INTERFACE_Metadata) == 0) {
    const char  *name;

    name = spa_dict_lookup (props, PW_KEY_METADATA_NAME);
    if (name == NULL || strcmp (name, "default") != 0) {
      return;
    }

    pwstate->metadata_id = id;
    pwstate->metadata = pw_registry_bind (pwstate->registry,
        id, type, PW_VERSION_METADATA, 0);
    mdextalloc (pwstate->metadata);
    pw_metadata_add_listener (pwstate->metadata,
        &pwstate->metadata_listener, &metadata_events, pwstate);

    return;
  }

  if (strcmp (type, PW_TYPE_INTERFACE_Device) == 0) {
    proxy = pw_registry_bind (pwstate->registry, id, type,
        version, sizeof (pwproxy_t));
    pd = pw_proxy_get_user_data (proxy);
    pd->type = PW_TYPE_DEVICE;
    pd->deviceid = id;
    pd->pwstate = pwstate;
    pd->pwsink = NULL;
    pd->proxy = proxy;
    pw_proxy_add_object_listener (proxy, &pd->object_listener, &device_events, pd);
    pw_map_insert_new (&pwstate->pwdevlist, pd);
    return;
  }

  /* node processing */

  if (strcmp (type, PW_TYPE_INTERFACE_Node) != 0) {
    return;
  }

  item = spa_dict_lookup_item (props, SPA_KEY_MEDIA_CLASS);
  if (item == NULL || strcmp (item->value, "Audio/Sink") != 0) {
    return;
  }

  if ((permissions & PW_PERM_W) != PW_PERM_W) {
    return;
  }

  sink = mdmalloc (sizeof (*sink));
  sink->internalid = pwstate->pwsinkcount;
  sink->nodeid = id;
  sink->name = NULL;
  sink->description = NULL;
  sink->info = NULL;

  item = spa_dict_lookup_item (props, PW_KEY_NODE_NAME);
  if (item != NULL) {
    sink->name = strdup (item->value);
  }
  item = spa_dict_lookup_item (props, PW_KEY_NODE_DESCRIPTION);
  if (item != NULL) {
    sink->description = strdup (item->value);
  }
  item = spa_dict_lookup_item (props, PW_KEY_DEVICE_ID);
  if (item != NULL) {
    sink->deviceid = atoi (item->value);
  }

  proxy = pw_registry_bind (pwstate->registry, id, type,
      version, sizeof (pwproxy_t));
  pd = pw_proxy_get_user_data (proxy);
  pd->type = PW_TYPE_NODE;
  pd->deviceid = 0;
  pd->pwstate = pwstate;
  pd->pwsink = sink;
  pd->proxy = proxy;
  pw_proxy_add_object_listener (proxy, &pd->object_listener, &node_events, pd);
//  pw_proxy_add_listener (proxy, &pd->proxy_listener, &proxy_events, pd);

  pw_map_for_each (&pd->pwstate->pwdevlist, pipewireMapDevice, sink);

  pw_map_insert_new (&pwstate->pwsinklist, sink);
  ++pwstate->pwsinkcount;
}

static int
pipewireGetSink (void *item, void *udata)
{
  pwsink_t      *sink = item;
  pwstate_t     *pwstate = udata;
  volsinklist_t *sinklist = pwstate->sinklist;
  int           idx;

  idx = sinklist->count;
  sinklist->sinklist [idx].defaultFlag = false;
  sinklist->sinklist [idx].idxNumber = sink->internalid;
  sinklist->sinklist [idx].name = mdstrdup (sink->name);
  sinklist->sinklist [idx].nmlen =
      strlen (sinklist->sinklist [idx].name);
  sinklist->sinklist [idx].description = mdstrdup (sink->description);
  if (strcmp (sinklist->sinklist [idx].name, pwstate->defname) == 0) {
    sinklist->sinklist [idx].defaultFlag = true;
  }

  ++sinklist->count;

  return 0;
}

static void
pipewireCheckMainLoop (void *udata, uint32_t id, int seq)
{
  pwstate_t   *pwstate = udata;

  if (id == PW_ID_CORE && seq == pwstate->coreseq) {
    pw_main_loop_quit (pwstate->pwmainloop);
  }
}

static void
pipewireRunOnce (pwstate_t *pwstate)
{
  static const struct pw_core_events core_events = {
      PW_VERSION_CORE_EVENTS,
      .done = pipewireCheckMainLoop,
  };
  struct spa_hook core_listener;

  pw_core_add_listener (pwstate->core, &core_listener, &core_events, pwstate);
  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, 0);

  pw_main_loop_run (pwstate->pwmainloop);

  spa_hook_remove (&core_listener);
}

static int
pipewireMetadataEvent (void *udata, uint32_t id,
    const char *key, const char *type, const char *value)
{
  pwstate_t *pwstate = udata;

  if (strcmp (key, "default.configured.audio.sink") == 0) {
    struct spa_json   iter[3];
    size_t            len;
    const char        *val;
    char              res [200];

    res [0] = '\0';
    spa_json_init (&iter[0], value, strlen (value));

    spa_json_enter (&iter[0], &iter[1]);
    len = spa_json_next (&iter[1], &val);

    spa_json_enter (&iter[1], &iter[2]);
    len = spa_json_next (&iter[2], &val);
    spa_json_get_string (&iter[2], res, sizeof (res));

    dataFree (pwstate->defname);
    pwstate->defname = mdstrdup (res);
  }

  return 0;
}

static void
pipewireNodeInfoEvent (void *udata, const struct pw_node_info *info)
{
  pwproxy_t     *pd = udata;
  uint32_t      param_id;
  const struct spa_type_info *ti;

  pd->pwsink->info = pw_node_info_update (pd->pwsink->info, info);
  if (info->props != NULL) {
    const struct spa_dict_item  *item;

    item = spa_dict_lookup_item (info->props, "card.profile.device");
    /* the card-profile-device is needed to set the volume */
    pd->pwsink->cardprofiledev = atoi (item->value);
  }

#if 0
  ti = spa_debug_type_find_short (spa_type_param, "Format");
  if (ti != NULL) {
    param_id = ti->type;
    pw_node_enum_params ((struct pw_node *) pd->proxy,
        0, param_id, 0, 1, NULL);
// Spa:Pod:Object:Param:Format:Audio:channels
  }
#endif

  pw_map_for_each (&pd->pwstate->pwdevlist, pipewireGetRoute, pd->pwsink);
}

static void
pipewireParamEvent (void *udata, int seq, uint32_t id,
    uint32_t index, uint32_t next, const struct spa_pod *param)
{
  pwproxy_t   *pd = udata;
  struct spa_pod_parser p;
  int         routeidx;
  int         dir;

  if (pd->type == PW_TYPE_DEVICE) {
    spa_pod_parser_pod (&p, param);
    spa_pod_parser_get_object (&p,
        SPA_TYPE_OBJECT_ParamRoute, NULL,
        SPA_PARAM_ROUTE_index, SPA_POD_Int (&routeidx),
        SPA_PARAM_ROUTE_direction, SPA_POD_Id (&dir));
    if (dir == SPA_DIRECTION_OUTPUT) {
      pd->pwsink->routeidx = routeidx;
    }
  }

  if (pd->type == PW_TYPE_NODE) {
    ;
  }
}

static int
pipewireGetRoute (void *item, void *udata)
{
  pwproxy_t     *pd = item;
  pwsink_t      *pwsink = udata;
  uint32_t      param_id;
  const struct spa_type_info *ti;

  if (pd->deviceid != pwsink->deviceid) {
    return 0;
  }

  ti = spa_debug_type_find_short (spa_type_param, "Route");
  if (ti != NULL) {
    param_id = ti->type;
    pw_device_enum_params ((struct pw_device *) pd->proxy,
        0, param_id, 0, 0, NULL);
  }

  return 1;
}


static int
pipewireMapDevice (void *item, void *udata)
{
  pwproxy_t     *pd = item;
  pwsink_t      *pwsink = udata;

  if (pd->deviceid != pwsink->deviceid) {
    return 0;
  }

  pd->pwsink = pwsink;
  pwsink->deviceProxy = pd;
  return 1;
}


#endif /* _hdr_pipewire_pipewire - have pipewire header */
