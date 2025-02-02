/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 *
 * https://gitlab.freedesktop.org/pipewire/pipewire/-/wikis/Migrate-PulseAudio
 *
 * pw-cli, pw-metadata
 * https://gitlab.freedesktop.org/pipewire/pipewire/-/blob/master/src/tools/
 *
 *  get a list of nodes
 *    pw-cli ls Node
 *  get the info for a node
 *    pw-cli i 78
 *      device.id
 *      card.profile.device
 *      media.class
 *  get the enumerations for a device
 *    pw e 74 Route
 *      index
 *      direction
 *      device (card.profile.device)
 *      channelVolumes
 *  set-volume for the route
 *    pw-cli set-param <device-id> \
 *       Route '{ index: <route-index>, device: <card-profile-device>, \
 *       props: { mute: false, channelVolumes: [ 0.5, 0.5 ] }, save: true }'
 *
 *  pipewire has their own pod, dict structures, json, standard c structures,
 *    a real mess.
 *  very inconsistent and strange api.
 *  the api is too low-level, and much parsing has to be done here.
 *
 */

#include "config.h"

#if _hdr_pipewire_pipewire && _hdr_spa_utils_jsonpod

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <math.h>

#define BDJ4_PW_DEBUG 0

#include <pipewire/pipewire.h>
#include <pipewire/keys.h>
#include <pipewire/extensions/metadata.h>
#include <spa/utils/keys.h>
#include <spa/param/param.h>
#include <spa/param/format.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/pod/filter.h>
#include <spa/utils/json.h>
#include <spa/utils/result.h>
#if BDJ4_PW_DEBUG
 // #include <spa/utils/type.h>
# include <pipewire/log.h>
# include <spa/utils/json-pod.h>
# include <spa/debug/pod.h>
#endif

#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

enum {
  BDJ4_PW_TYPE_DEVICE,
  BDJ4_PW_TYPE_NODE,
  BDJ4_PWSTATE,
  BDJ4_PWSINK,
  BDJ4_PWPROXY,
};

enum {
  BDJ4_PW_MAX_CHAN = 5,
};

typedef struct pwproxy pwproxy_t;
typedef struct pwsink pwsink_t;
typedef struct pwstate pwstate_t;

typedef struct pwstate {
  int                   ident;
  struct pw_thread_loop *pwloop;
  struct pw_context     *context;
  struct pw_core        *core;
  struct pw_registry    *registry;
  struct pw_metadata    *metadata;
  struct spa_hook       core_listener;
  struct spa_hook       registry_listener;
  struct spa_hook       metadata_listener;
  struct pw_map         pwdevlist;
  struct pw_map         pwsinklist;
  volsinklist_t         *sinklist;
  char                  *defsinkname;
  const char            *findname;
  uint32_t              findid;
  pwsink_t              *currsink;
  pwsink_t              *findsink;
  pwproxy_t             *finddev;
  uint32_t              metadata_id;
  int                   pwsinkcount;
  int                   coreseq;
  int                   devidcount;
  bool                  changed;
  bool                  initialized;
} pwstate_t;

typedef struct pwsink {
  int                   ident;
  struct pw_node_info   *info;
  pwproxy_t             *pwdevproxy;
  struct pw_proxy       *proxy;
  char                  *description;
  char                  *name;
  int                   currvolume;   /* percentage as an int */
  uint32_t              nodeid;
  uint32_t              deviceid;
  uint32_t              routedevid;
  int                   internalid;
  int                   routeidx;
  int                   channels;
  int                   available;
} pwsink_t;

typedef struct pwproxy {
  int                   ident;
  pwstate_t             *pwstate;
  pwsink_t              *pwsink;
  struct spa_hook       object_listener;
  struct spa_hook       proxy_listener;
  struct pw_proxy       *proxy;
  uint32_t              deviceid;
  int                   internalid;
  int                   type;
  int                   available;
} pwproxy_t;

static pwstate_t *pipewireInit (void);
static void pipewireWaitForInit (pwstate_t *pwstate);
static void pipewireCoreEvent (void *udata, uint32_t id, int seq);
static void pipewireRegistryEvent (void *udata, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props);
static void pipewireRegistryRemoveEvent (void *udata, uint32_t id);
static int pipewireMetadataEvent (void *udata, uint32_t id, const char *key, const char *type, const char *value);
static void pipewireNodeInfoEvent (void *udata, const struct pw_node_info *info);
static void pipewireParamEvent (void *udata, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param);
static int pipewireMapDevice (void *item, void *udata);
static int pipewireGetRoute (void *item, void *udata);
static int pipewireCheckSink (void *item, void *udata);
static void pipewireFreeProxy (void *udata);
static int pipewireFreeDevice (void *item, void *udata);
static int pipewireFreeNode (void *item, void *udata);
static void pipewireGetCurrentSink (pwstate_t *pwstate, const char *sinkname);
static int pipewireFindSinkByName (void *item, void *udata);
static int pipewireFindSinkByID (void *item, void *udata);
static int pipewireFindDevByID (void *item, void *udata);
static void pipewireSetVolume (pwstate_t *pwstate, int vol);
#if BDJ4_PW_DEBUG
static int pipewireDumpSink (void *item, void *udata);
#endif

static const struct pw_core_events core_events = {
  .version = PW_VERSION_CORE_EVENTS,
  .done = pipewireCoreEvent,
};

static const struct pw_registry_events registry_events = {
  .version = PW_VERSION_REGISTRY_EVENTS,
  .global = pipewireRegistryEvent,
  .global_remove = pipewireRegistryRemoveEvent,
};

static const struct pw_metadata_events metadata_events = {
  .version = PW_VERSION_METADATA_EVENTS,
  .property = pipewireMetadataEvent,
};

static const struct pw_node_events node_events = {
  .version = PW_VERSION_NODE_EVENTS,
  .info = pipewireNodeInfoEvent,
  .param = pipewireParamEvent,
};

static const struct pw_device_events device_events = {
  .version = PW_VERSION_DEVICE_EVENTS,
  .param = pipewireParamEvent,
};

static const struct pw_proxy_events proxy_events = {
  .version = PW_VERSION_PROXY_EVENTS,
  .destroy = pipewireFreeProxy,
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
  pwstate_t   *pwstate;

  if (udata == NULL || *udata == NULL) {
    return;
  }

  pwstate = *udata;

  if (pwstate->pwloop != NULL) {
    pw_thread_loop_stop (pwstate->pwloop);
  }

  if (pwstate->metadata != NULL) {
    spa_hook_remove (&pwstate->metadata_listener);
    mdextfree (pwstate->metadata);
    pw_proxy_destroy ((struct pw_proxy *) pwstate->metadata);
    pwstate->metadata = NULL;
  }

  if (pwstate->registry != NULL) {
    spa_hook_remove (&pwstate->registry_listener);
    mdextfree (pwstate->registry);
    pw_proxy_destroy ((struct pw_proxy *) pwstate->registry);
    pwstate->registry = NULL;
  }

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

  /* the props are freed by the context-destroy */

  if (pwstate->pwloop != NULL) {
    mdextfree (pwstate->pwloop);
    pw_thread_loop_destroy (pwstate->pwloop);
    pwstate->pwloop = NULL;
  }

  pw_map_for_each (&pwstate->pwdevlist, pipewireFreeDevice, pwstate);
  pw_map_clear (&pwstate->pwdevlist);
  pw_map_for_each (&pwstate->pwsinklist, pipewireFreeNode, pwstate);
  pw_map_clear (&pwstate->pwsinklist);

  dataFree (pwstate->defsinkname);
  pwstate->defsinkname = NULL;

  pw_deinit ();
  mdfree (pwstate);
  *udata = NULL;

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
    pipewireWaitForInit (pwstate);

    *udata = pwstate;

#if BDJ4_PW_DEBUG
    pw_thread_loop_lock (pwstate->pwloop);
    pw_map_for_each (&pwstate->pwsinklist, pipewireDumpSink, pwstate);
    pw_thread_loop_unlock (pwstate->pwloop);
#endif
  }

  if (action == VOL_GETSINKLIST) {
    /* unfortunately, linux pulseaudio/pipewire does not have a */
    /* concept of use-the-default-output */

    pw_thread_loop_lock (pwstate->pwloop);

    dataFree (sinklist->defname);
    sinklist->defname = NULL;
    sinklist->sinklist = NULL;
    sinklist->count = 0;
    sinklist->sinklist = mdrealloc (sinklist->sinklist,
        pwstate->pwsinkcount * sizeof (volsinkitem_t));

    pwstate->sinklist = sinklist;
    pw_map_for_each (&pwstate->pwsinklist, pipewireCheckSink, pwstate);

    pw_thread_loop_unlock (pwstate->pwloop);
    return 0;
  }

  if (action == VOL_CHK_SINK) {
    bool    orig;

    /* leave the changed flag set until the user fetches it */
    pw_thread_loop_lock (pwstate->pwloop);
    orig = pwstate->changed;
    pw_thread_loop_unlock (pwstate->pwloop);
    pwstate->changed = false;
    return orig;
  }

  pw_thread_loop_lock (pwstate->pwloop);
  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);
  pw_thread_loop_unlock (pwstate->pwloop);

  pipewireGetCurrentSink (pwstate, sinkname);

  if (action == VOL_SET) {
    pipewireSetVolume (pwstate, *vol);
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    if (pwstate != NULL) {
      pw_thread_loop_lock (pwstate->pwloop);
      if (pwstate->currsink != NULL) {
        *vol = pwstate->currsink->currvolume;
      }
      pw_thread_loop_unlock (pwstate->pwloop);
    }
  }

  return *vol;
}

/* internal routines */

static pwstate_t *
pipewireInit (void)
{
  pwstate_t *pwstate = NULL;

  mdebugSubTag ("volpipewire");
  pw_init (0, NULL);

  pwstate = mdmalloc (sizeof (pwstate_t));
  pwstate->ident = BDJ4_PWSTATE;
  pwstate->pwloop = NULL;
  pwstate->context = NULL;
  pwstate->core = NULL;
  pwstate->registry = NULL;
  pwstate->metadata = NULL;
  pwstate->defsinkname = NULL;
  pwstate->findname = NULL;
  pwstate->findid = 0;
  pwstate->currsink = NULL;
  pwstate->findsink = NULL;
  pwstate->finddev = NULL;
  pwstate->pwsinkcount = 0;
  pwstate->coreseq = 0;
  pwstate->devidcount = 0;
  pwstate->changed = false;
  pwstate->initialized = false;

  pwstate->pwloop = pw_thread_loop_new (NULL, NULL);
  mdextalloc (pwstate->pwloop);

  pwstate->context = pw_context_new (
      pw_thread_loop_get_loop (pwstate->pwloop),
      NULL, 0);
  mdextalloc (pwstate->context);

  pwstate->core = pw_context_connect (pwstate->context, NULL, 0);
  mdextalloc (pwstate->core);

  pwstate->registry = pw_core_get_registry (pwstate->core, PW_VERSION_REGISTRY, 0);
  mdextalloc (pwstate->registry);

  pw_map_init (&pwstate->pwdevlist, 8, 1);
  pw_map_init (&pwstate->pwsinklist, 4, 1);

  pw_core_add_listener (pwstate->core,
      &pwstate->core_listener, &core_events, pwstate);
  pw_registry_add_listener (pwstate->registry,
      &pwstate->registry_listener, &registry_events, pwstate);

  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

  pw_thread_loop_start (pwstate->pwloop);

  return pwstate;
}

static void
pipewireWaitForInit (pwstate_t *pwstate)
{
  pw_thread_loop_lock (pwstate->pwloop);
  while (pwstate->initialized == false) {
    pw_thread_loop_timed_wait (pwstate->pwloop, 10);
  }
  pw_thread_loop_unlock (pwstate->pwloop);
}

static void
pipewireCoreEvent (void *udata, uint32_t id, int seq)
{
  pwstate_t   *pwstate = udata;

  if (id != PW_ID_CORE || seq != pwstate->coreseq) {
    return;
  }

  /* after the first initialization, core events no longer need to be tracked */
  spa_hook_remove (&pwstate->core_listener);

  pwstate->initialized = true;
  pw_thread_loop_signal (pwstate->pwloop, false);
}

/* all pipewire events run in the thread loop are locked by pipewire */

static void
pipewireRegistryEvent (void *udata, uint32_t id,
    uint32_t permissions, const char *type, uint32_t version,
    const struct spa_dict *props)
{
  pwstate_t                   *pwstate = udata;
  const struct spa_dict_item  *item;
  pwsink_t                    *sink;
  struct pw_proxy             *proxy;
  pwproxy_t                   *pd;

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: reg-event: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

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
        id, type, version, 0);
    mdextalloc (pwstate->metadata);
    pw_metadata_add_listener (pwstate->metadata,
        &pwstate->metadata_listener, &metadata_events, pwstate);
    pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

    return;
  }

  if (strcmp (type, PW_TYPE_INTERFACE_Device) == 0) {
    proxy = pw_registry_bind (pwstate->registry, id, type,
        version, sizeof (pwproxy_t));
    mdextalloc (proxy);
    pd = pw_proxy_get_user_data (proxy);
    pd->ident = BDJ4_PWPROXY;
    pd->type = BDJ4_PW_TYPE_DEVICE;
    pd->deviceid = id;
    pd->pwstate = pwstate;
    pd->pwsink = NULL;
    pd->proxy = proxy;
    pd->internalid = pwstate->devidcount;
    pd->available = true;
    pw_proxy_add_object_listener (proxy, &pd->object_listener, &device_events, pd);
    pw_proxy_add_listener (proxy, &pd->proxy_listener, &proxy_events, pd);
    /* the node-ids change all the time, there's no point in trying to */
    /* find the original entry, just re-add it. */
    pw_map_insert_at (&pwstate->pwdevlist, pd->internalid, pd);
    ++pwstate->devidcount;

    pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

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
  sink->ident = BDJ4_PWSINK;
  sink->internalid = pwstate->pwsinkcount;
  sink->nodeid = id;
  sink->name = NULL;
  sink->description = NULL;
  sink->info = NULL;
  sink->channels = 0;
  sink->available = true;

  item = spa_dict_lookup_item (props, PW_KEY_NODE_NAME);
  if (item != NULL) {
    sink->name = mdstrdup (item->value);
  }
  item = spa_dict_lookup_item (props, PW_KEY_NODE_DESCRIPTION);
  if (item != NULL) {
    sink->description = mdstrdup (item->value);
  }
  item = spa_dict_lookup_item (props, PW_KEY_DEVICE_ID);
  if (item != NULL) {
    sink->deviceid = atoi (item->value);
  }

  ++pwstate->pwsinkcount;
  /* the node-ids change all the time, there's no point in trying to */
  /* find the original entry, just re-add it. */
  pw_map_insert_at (&pwstate->pwsinklist, sink->internalid, sink);

  proxy = pw_registry_bind (pwstate->registry, sink->nodeid,
      PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, sizeof (pwproxy_t));
  mdextalloc (proxy);
  pd = pw_proxy_get_user_data (proxy);
  pd->ident = BDJ4_PWPROXY;
  pd->type = BDJ4_PW_TYPE_NODE;
  pd->deviceid = sink->nodeid;
  pd->pwstate = pwstate;
  pd->pwsink = sink;
  pd->proxy = proxy;
  pd->internalid = 0;
  sink->proxy = proxy;

  pw_proxy_add_object_listener (proxy, &pd->object_listener, &node_events, pd);
  pw_proxy_add_listener (proxy, &pd->proxy_listener, &proxy_events, pd);

  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

  pw_map_for_each (&pwstate->pwdevlist, pipewireMapDevice, sink);
}

static void
pipewireRegistryRemoveEvent (void *udata, uint32_t id)
{
  pwstate_t   *pwstate = udata;

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: reg-event: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  /* it is unknown whether the id is a node, a device, or something else */

  pwstate->findid = id;
  pwstate->findsink = NULL;
  pwstate->finddev = NULL;
  pw_map_for_each (&pwstate->pwsinklist, pipewireFindSinkByID, pwstate);
  if (pwstate->findsink != NULL) {
    pwstate->findsink->available = false;
    pwstate->findsink->nodeid = 0;
    if (pwstate->findsink == pwstate->currsink) {
      pwstate->changed = true;
    }
  } else {
    pw_map_for_each (&pwstate->pwdevlist, pipewireFindDevByID, pwstate);
    if (pwstate->finddev != NULL) {
      pwstate->finddev->available = false;
      pwstate->finddev->deviceid = 0;
    }
  }
}

static int
pipewireMetadataEvent (void *udata, uint32_t id,
    const char *key, const char *type, const char *value)
{
  pwstate_t *pwstate = udata;

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: metadata-event: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (strcmp (key, "default.configured.audio.sink") == 0) {
    struct spa_json   iter[3];
    size_t            len;
    const char        *val;
    char              res [200];

    /* is there another way to deal with this stuff? */
    res [0] = '\0';
    spa_json_init (&iter[0], value, strlen (value));

    spa_json_enter (&iter[0], &iter[1]);
    len = spa_json_next (&iter[1], &val);

    spa_json_enter (&iter[1], &iter[2]);
    len = spa_json_next (&iter[2], &val);
    spa_json_get_string (&iter[2], res, sizeof (res));

    if (pwstate->defsinkname != NULL &&
        strcmp (pwstate->defsinkname, res) != 0) {
      pwstate->changed = true;
    }
    if (pwstate->defsinkname == NULL || pwstate->changed) {
      dataFree (pwstate->defsinkname);
      pwstate->defsinkname = mdstrdup (res);
    }
  }

  return 0;
}

static void
pipewireNodeInfoEvent (void *udata, const struct pw_node_info *info)
{
  pwproxy_t     *pd = udata;
  pwsink_t      *pwsink = pd->pwsink;
  pwstate_t     *pwstate = pd->pwstate;
  uint32_t      param_id;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: node-info-event: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: node-info-event-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: node-info-event-state: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSTATE);
    exit (1);
  }

  pwsink->info = pw_node_info_update (pwsink->info, info);
  mdextalloc (pwsink->info);
  if (info->props != NULL) {
    const struct spa_dict_item  *item;

    item = spa_dict_lookup_item (info->props, "card.profile.device");
    if (item != NULL) {
      /* the card-profile-device is needed to set the volume */
      pwsink->routedevid = atoi (item->value);
    }
  }

  param_id = SPA_PARAM_EnumFormat;
  pw_node_enum_params ((struct pw_node *) pwsink->proxy,
      0, param_id, 0, 50, NULL);

  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

  pw_map_for_each (&pd->pwstate->pwdevlist, pipewireGetRoute, pwsink);
}

static void
pipewireParamEvent (void *udata, int seq, uint32_t id,
    uint32_t index, uint32_t next, const struct spa_pod *param)
{
  pwproxy_t   *pd = udata;
  pwsink_t    *pwsink = pd->pwsink;
  struct spa_pod_parser p;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: param-event: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  spa_pod_parser_pod (&p, param);
#if BDJ4_PW_DEBUG
  // spa_debug_pod (4, NULL, param); fflush (stdout);
#endif

  if (pd->type == BDJ4_PW_TYPE_DEVICE) {
    int                 routeidx = 0;
    uint32_t            dir = SPA_DIRECTION_INPUT;
    struct spa_pod_parser pp;
    struct spa_pod      *props;
    struct spa_pod      *vols;
    float               volume [BDJ4_PW_MAX_CHAN];
    int                 nvol;
    bool                mute = false;

    for (int i = 0; i < BDJ4_PW_MAX_CHAN; ++i) {
      volume [i] = 0.0;
    }

#if BDJ4_PW_DEBUG
    // spa_debug_pod (4, NULL, param); fflush (stdout);
#endif
    spa_pod_parser_get_object (&p,
        SPA_TYPE_OBJECT_ParamRoute, NULL,
        SPA_PARAM_ROUTE_index, SPA_POD_Int (&routeidx),
        SPA_PARAM_ROUTE_direction, SPA_POD_Id (&dir),
        SPA_PARAM_ROUTE_props, SPA_POD_PodObject (&props));
    if (dir == SPA_DIRECTION_OUTPUT) {
      pwsink->routeidx = routeidx;

#if BDJ4_PW_DEBUG
      // spa_debug_pod (6, NULL, props); fflush (stdout);
#endif
      spa_pod_parser_pod (&pp, props);
      spa_pod_parser_get_object (&pp,
          SPA_TYPE_OBJECT_Props, NULL,
          SPA_PROP_channelVolumes, SPA_POD_Pod (&vols),
          SPA_PROP_mute, SPA_POD_Bool (&mute));

#if BDJ4_PW_DEBUG
      // spa_debug_pod (6, NULL, vols);
#endif
      nvol = spa_pod_copy_array (vols, SPA_TYPE_Float, volume, BDJ4_PW_MAX_CHAN);

      /* where is the cubic root documented? */
      /* this states that the volume is linear: */
      /*   https://gitlab.freedesktop.org/pipewire/pipewire/-/issues/3623 */
      /* this is very odd */
      pwsink->currvolume =
          (int) round (pow ((double) volume [0], (1.0 / 3.0)) * 100.0);
    }
  }

  if (pd->type == BDJ4_PW_TYPE_NODE) {
    int       chan = 0;
    uint32_t  mediatype = 0;

    spa_pod_parser_get_object (&p,
        SPA_TYPE_OBJECT_Format, NULL,
        SPA_FORMAT_AUDIO_channels, SPA_POD_Int (&chan),
        SPA_FORMAT_mediaType, SPA_POD_Id (&mediatype));
    if (mediatype == SPA_MEDIA_TYPE_audio &&
        chan >= 2) {
      pwsink->channels = chan;
    }
  }
}

/* map-device is called from within an event, */
/* the thread loop is locked by pipewire */

static int
pipewireMapDevice (void *item, void *udata)
{
  pwproxy_t     *pd = item;
  pwsink_t      *pwsink = udata;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: map-dev: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: map-dev-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pd->available == false) {
    return 0;
  }

  if (pd->deviceid != pwsink->deviceid) {
    return 0;
  }

  pd->pwsink = pwsink;
  pwsink->pwdevproxy = pd;
  return 1;
}

/* get-route is called from within an event, */
/* the thread loop is locked by pipewire */

static int
pipewireGetRoute (void *item, void *udata)
{
  pwproxy_t   *pd = item;
  pwsink_t    *pwsink = udata;
  pwstate_t   *pwstate = pd->pwstate;
  uint32_t    param_id;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: get-route: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: get-route-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: get-route-state: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (pd->available == false) {
    return 0;
  }

  if (pd->deviceid != pwsink->deviceid) {
    return 0;
  }

  param_id = SPA_PARAM_Route;
  pw_device_enum_params ((struct pw_device *) pd->proxy,
      0, param_id, 0, 0, NULL);

  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

  return 1;
}

/* get-sink loads the sinklist from the pipewire sink list */
/* the thread loop is locked by the caller */

static int
pipewireCheckSink (void *item, void *udata)
{
  pwsink_t      *pwsink = item;
  pwstate_t     *pwstate = udata;
  volsinklist_t *sinklist = pwstate->sinklist;
  int           idx;

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: get-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: get-sink-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (pwsink->available == false) {
    return 0;
  }

  if (pwsink->channels < 2) {
    return 0;
  }

  if (pwsink->pwdevproxy != NULL) {
    if (pwsink->pwdevproxy->available == false) {
      return 0;
    }
  }

  idx = sinklist->count;
  sinklist->sinklist [idx].defaultFlag = false;
  sinklist->sinklist [idx].idxNumber = pwsink->internalid;
  sinklist->sinklist [idx].name = mdstrdup (pwsink->name);
  sinklist->sinklist [idx].nmlen =
      strlen (sinklist->sinklist [idx].name);
  sinklist->sinklist [idx].description = mdstrdup (pwsink->description);
  if (strcmp (sinklist->sinklist [idx].name, pwstate->defsinkname) == 0) {
    sinklist->sinklist [idx].defaultFlag = true;
    dataFree (sinklist->defname);
    sinklist->defname = mdstrdup (pwstate->defsinkname);
  }

  ++sinklist->count;

  return 0;
}

static void
pipewireFreeProxy (void *udata)
{
  pwproxy_t     *pd = udata;
  pwstate_t     *pwstate = pd->pwstate;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: free-proxy: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: free-proxy-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  spa_hook_remove (&pd->object_listener);
  spa_hook_remove (&pd->proxy_listener);
  if (pd->type == BDJ4_PW_TYPE_NODE) {
    pd->pwsink->proxy = NULL;
  }

  pd->proxy = NULL;
}

static int
pipewireFreeDevice (void *item, void *udata)
{
  pwproxy_t     *pd = item;
  pwstate_t     *pwstate = udata;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: free-device: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: free-device-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  pw_map_insert_at (&pwstate->pwdevlist, pd->internalid, NULL);
  mdextfree (pd->proxy);
  pw_proxy_destroy (pd->proxy);

  return 0;
}

static int
pipewireFreeNode (void *item, void *udata)
{
  pwsink_t      *pwsink = item;
  pwstate_t     *pwstate = udata;

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: free-node: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: free-node-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  pw_map_insert_at (&pwstate->pwsinklist, pwsink->internalid, NULL);
  dataFree (pwsink->description);
  dataFree (pwsink->name);

  mdextfree (pwsink->info);
  pw_node_info_free (pwsink->info);

  mdextfree (pwsink->proxy);
  pw_proxy_destroy (pwsink->proxy);

  dataFree (pwsink);

  return 0;
}

static void
pipewireGetCurrentSink (pwstate_t *pwstate, const char *sinkname)
{
  bool    usedefault = false;

  pw_thread_loop_lock (pwstate->pwloop);

  if (pwstate->currsink != NULL && pwstate->changed == false) {
    pw_thread_loop_unlock (pwstate->pwloop);
    return;
  }

  pwstate->findname = sinkname;
  if (sinkname == NULL || ! *sinkname) {
    pwstate->findname = pwstate->defsinkname;
    usedefault = true;
  }

  if (pwstate->currsink == NULL || pwstate->changed) {
    pwstate->findsink = NULL;
    pw_map_for_each (&pwstate->pwsinklist, pipewireFindSinkByName, pwstate);
  }

  if (pwstate->findsink == NULL && usedefault == false) {
    /* try and find the default sink */
    pwstate->findname = pwstate->defsinkname;
    pwstate->findsink = NULL;
    pw_map_for_each (&pwstate->pwsinklist, pipewireFindSinkByName, pwstate);
  }

  pwstate->currsink = pwstate->findsink;

  pw_thread_loop_unlock (pwstate->pwloop);
}

static int
pipewireFindSinkByName (void *item, void *udata)
{
  pwsink_t      *pwsink = item;
  pwstate_t     *pwstate = udata;

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: find-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: find-sink-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (pwsink->available == false) {
    return 0;
  }

  if (strcmp (pwstate->findname, pwsink->name) == 0) {
    pwstate->findsink = pwsink;
    return 1;
  }

  return 0;
}


static int
pipewireFindSinkByID (void *item, void *udata)
{
  pwsink_t      *pwsink = item;
  pwstate_t     *pwstate = udata;

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: find-sink-id: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: find-sink-id-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (pwsink->available == false) {
    return 0;
  }

  if (pwsink->nodeid == pwstate->findid) {
    pwstate->findsink = pwsink;
    return 1;
  }

  return 0;
}

static int
pipewireFindDevByID (void *item, void *udata)
{
  pwproxy_t     *pd = item;
  pwstate_t     *pwstate = udata;

  if (pd == NULL || pd->ident != BDJ4_PWPROXY) {
    fprintf (stderr, "ERR: find-dev: invalid struct %d(%d)\n", pd->ident, BDJ4_PWPROXY);
    exit (1);
  }

  if (pwstate == NULL || pwstate->ident != BDJ4_PWSTATE) {
    fprintf (stderr, "ERR: find-dev-state: invalid struct %d(%d)\n", pwstate->ident, BDJ4_PWSTATE);
    exit (1);
  }

  if (pd->available == false) {
    return 0;
  }

  if (pd->deviceid == pwstate->findid) {
    pwstate->finddev = pd;
    return 1;
  }

  return 0;
}


static void
pipewireSetVolume (pwstate_t *pwstate, int vol)
{
  char            pbuff [1024];
  char            ppbuff [1024];
  struct spa_pod  *pod = NULL;
  struct spa_pod  *ppod = NULL;
  struct spa_pod_builder b;
  float           vols [BDJ4_PW_MAX_CHAN];
  float           tvol;
  pwsink_t        *pwsink = NULL;
  pwproxy_t       *pd = NULL;

  pw_thread_loop_lock (pwstate->pwloop);

  pwsink = pwstate->currsink;

  if (pwsink == NULL) {
    pw_thread_loop_unlock (pwstate->pwloop);
    return;
  }

  tvol = pow (((double) vol / 100.0), 3.0);
  for (int i = 0; i < pwsink->channels; ++i) {
    vols [i] = tvol;
  }

/*
 * works via pw-cli, but very sensitive to the json format
 * set-params 49 Route
 *    '{ index: 3, device: 6, save: true, props: { channelVolumes: [ 0.125, 0.125 ], mute: false }}'
 *
 * It is unknown what 'save' does, and whether it is needed (left out).
 *
 * Object: size 160, type Spa:Pod:Object:Param:Route (262153), id Spa:Enum:ParamId:Route (13)
 *   Prop: key Spa:Pod:Object:Param:Route:index (1), flags 00000000
 *     Int 3
 *   Prop: key Spa:Pod:Object:Param:Route:device (3), flags 00000000
 *     Int 6
 *   Prop: key Spa:Pod:Object:Param:Route:save (13), flags 00000000
 *     Bool true
 *   Prop: key Spa:Pod:Object:Param:Route:props (10), flags 00000000
 *     Object: size 64, type Spa:Pod:Object:Param:Props (262146), id Spa:Enum:ParamId:Route (13)
 *       Prop: key Spa:Pod:Object:Param:Props:channelVolumes (65544), flags 00000000
 *         Array: child.size 4, child.type Spa:Float
 *           Float 0.125000
 *           Float 0.125000
 *       Prop: key Spa:Pod:Object:Param:Props:mute (65540), flags 00000000
 *         Bool false
 *
 */

  spa_pod_builder_init (&b, ppbuff, sizeof (ppbuff));
  ppod = spa_pod_builder_add_object (&b,
      SPA_TYPE_OBJECT_Props, SPA_PARAM_Route,
      SPA_PROP_channelVolumes,
          SPA_POD_Array (sizeof (float), SPA_TYPE_Float, pwsink->channels, vols),
      SPA_PROP_mute, SPA_POD_Bool (false),
      NULL);
#if BDJ4_PW_DEBUG
  // spa_debug_pod (0, NULL, ppod); fflush (stdout);
#endif

  spa_pod_builder_init (&b, pbuff, sizeof (pbuff));
  pod = spa_pod_builder_add_object (&b,
      SPA_TYPE_OBJECT_ParamRoute, SPA_PARAM_Route,
      SPA_PARAM_ROUTE_index, SPA_POD_Int (pwsink->routeidx),
      SPA_PARAM_ROUTE_device, SPA_POD_Int (pwsink->routedevid),
      SPA_PARAM_ROUTE_props, SPA_POD_PodObject (ppod),
      NULL);
#if BDJ4_PW_DEBUG
  spa_debug_pod (0, NULL, pod); fflush (stdout);
#endif

  pd = pwsink->pwdevproxy;
  pw_device_set_param ((struct pw_device *) pd->proxy, SPA_PARAM_Route, 0, pod);
  pwstate->coreseq = pw_core_sync (pwstate->core, PW_ID_CORE, pwstate->coreseq);

  pw_thread_loop_unlock (pwstate->pwloop);
}


#if BDJ4_PW_DEBUG

static int
pipewireDumpSink (void *item, void *udata)
{
  pwsink_t      *pwsink = item;

  if (pwsink == NULL || pwsink->ident != BDJ4_PWSINK) {
    fprintf (stderr, "ERR: dump-sink: invalid struct %d(%d)\n", pwsink->ident, BDJ4_PWSINK);
    exit (1);
  }

  if (pwsink->available == false) {
    return 0;
  }

  fprintf (stderr, "--   int-id: %d\n", pwsink->internalid);
  fprintf (stderr, "    node-id: %d\n", pwsink->nodeid);
  fprintf (stderr, "     dev-id: %d\n", pwsink->deviceid);
  fprintf (stderr, "  route-dev: %d\n", pwsink->routedevid);
  fprintf (stderr, "      route: %d\n", pwsink->routeidx);
  fprintf (stderr, "   channels: %d\n", pwsink->channels);
  fprintf (stderr, "       desc: %s\n", pwsink->description);
  fprintf (stderr, "       name: %s\n", pwsink->name);
  fprintf (stderr, "       info: %s\n", pwsink->info == NULL ? "ng" : "ok");
  fprintf (stderr, "  dev-proxy: %s\n", pwsink->pwdevproxy == NULL ? "ng" : "ok");
  fprintf (stderr, "      proxy: %s\n", pwsink->proxy == NULL ? "ng" : "ok");

  return 0;
}

#endif /* BDJ4_PW_DEBUG */

#endif /* _hdr_pipewire_pipewire - have pipewire header */
