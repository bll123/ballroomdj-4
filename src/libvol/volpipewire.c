#include "config.h"

#if _hdr_pipewire_pipewire

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

/*
 * has an audio fade example:
 * https://docs.pipewire.org/spa_2examples_2adapter-control_8c-example.html
 * possible sink example:
 * https://docs.pipewire.org/spa_2examples_2example-control_8c-example.html
 * search for pipewire-pulse source
 *  and check how get-sink-list, set-sink, volume controls work.
 * pw-metadata shows the defaults
 */

#include <pipewire/pipewire.h>

#include "mdebug.h"
#include "volsink.h"
#include "volume.h"

typedef struct {
  struct pw_main_loop   *pwmainloop;
  struct pw_properties  *props;
  struct pw_context     *context;
  struct pw_core        *core;
  struct pw_registry    *registry;
  struct spa_hook       registry_listener;
} pwstate_t;

static const struct pw_registry_events registry_events = {
  PW_VERSION_REGISTRY_EVENTS,
  .global = registry_event_global,
};

static void pipewireSigHandler (void *udata, int signal_number);

const char *
voliDesc (void)
{
  return "PipeWire";
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
    if (pwstate->core != NULL) {
      mdextfree (pwstate->core);
      pw_core_disconnect (pwstate->core);
      pwstate->core = NULL;
    }
    if (pwstate->context != NULL) {
      mdextfree (context);
      pw_context_destroy (context);
      pwstate->context = NULL;
    }
    if (pwstate->pwmainloop != NULL) {
      mdextfree (pwstate->pwmainloop);
      pw_main_loop_destroy (pwstate->pwmainloop);
      pwstate->pwmainloop = NULL;
    }
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
    pw_init (0, NULL);
    pwstate = mdmalloc (sizeof (pwstate_t));
    pwstate->pwmainloop = NULL;
    pwstate->props = NULL;
    pwstate->context = NULL;
    pwstate->core = NULL;

    pwstate->pwmainloop = pw_main_loop_new (NULL);
    mdextalloc (pwstate->pamainloop);
    pw_loop_add_signal (pw_main_loop_get_loop (pwstate->pwmainloop),
        SIGINT, pipewireSigHandler, pwstate);
    pw_loop_add_signal (pw_main_loop_get_loop (pwstate->pwmainloop),
        SIGTERM, pipewireSigHandler, pwstate);

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

    pwstate->registry = pw_core_get_registry (pwstate->core, PW_VERSION_REGISTRY, 0);
    mdextalloc (pwstate->registry);

    pw_registry_add_listener (pwstate->registry,
        &pwstate->registry_listener, &registry_events, pwstate);
  }

  roundtrip (pwstate->core, pwstate->pwmainloop);

  if (action == VOL_GETSINKLIST) {
    return 0;
  }

  if (action == VOL_SET_SYSTEM_DFLT) {
    return 0;
  }

  if (action == VOL_SET) {
  }

  if (vol != NULL && (action == VOL_SET || action == VOL_GET)) {
    return *vol;
  }

  return 0;
}

static void
pipewireSigHandler (void *udata, int signal_number)
{
  pwstate_t   *pwstate = udata;

  pw_main_loop_quit (psstate->pwmainloop);
}

#endif /* _hdr_pipewire_pipewire have pipewire header */
