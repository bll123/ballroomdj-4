/*
 * references:
 *   https://github.com/alsa-project/alsa-utils/
 *   https://stackoverflow.com/questions/6787318/set-alsa-master-volume-from-c-code#6787957
 *   https://github.com/alsa-project/alsa-utils/blob/master/alsamixer/mixer_controls.c
 *   https://www.alsa-project.org/alsa-doc/alsa-lib/files.html
 *
 * To do:
 *   very simplistic right now, sinks are not working.
 *   want to be able to differentiate between playback and capture devices.
 *   vlc uses the alsa PCM list (aplay --list-pcms).
 *     want to be able to set the default device for vlc.
 *   would be nice to know which of the PCMs are usable.
 */

#include "config.h"

#if _hdr_alsa_asoundlib

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>
#include <math.h>
#include <regex.h>
#include <alsa/asoundlib.h>

#include "bdj4intl.h"
#include "volume.h"

typedef struct cardlist {
  int           cardnumber;
  char          *id;
  char          *name;
} cardlist_t;

void
volumeDisconnect (void) {
  return;
}

int
volumeProcess (volaction_t action, char *sinkname,
    int *vol, volsinklist_t *sinklist)
{
  static snd_mixer_t    *handle = NULL;
  snd_mixer_elem_t      *elem;
  snd_mixer_selem_id_t  *sid;
  char                  card [40];
  int                   err;

  snd_mixer_selem_id_alloca (&sid);

  if (action == VOL_GETSINKLIST) {
    /* only use the alsa default device */
  } else {
    /* getvolume or setvolume */
    char                *stridx;
    char                *cardid;
    char                *volsink;
    long                min;
    long                max;
    long                lvol;

    cardid = strdup (sinkname);
    assert (cardid != NULL);

    stridx = index (cardid, ' ');
    if (stridx == NULL) {
      fprintf (stderr, "Invalid sinkname %s\n", sinkname);
      return -1;
    }
    *stridx = '\0';
    snprintf (card, sizeof (card), "hw:%s", cardid);
    volsink = stridx + 1;

    if ((err = snd_mixer_open (&handle, 0)) < 0) {
      free (cardid);
      fprintf (stderr, "Unable to open mixer: %s\n", snd_strerror(err));
      return -1;
    }
    if ((err = snd_mixer_attach (handle, card)) < 0) {
      free (cardid);
      fprintf (stderr, "Unable to attach mixer: %s\n", snd_strerror(err));
      snd_mixer_close (handle);
      return -1;
    }
    if ((err = snd_mixer_selem_register (handle, NULL, NULL)) < 0) {
      free (cardid);
      fprintf (stderr, "Unable to register mixer: %s\n", snd_strerror(err));
      snd_mixer_close (handle);
      return -1;
    }
    err = snd_mixer_load (handle);
    if (err < 0) {
      free (cardid);
      fprintf (stderr, "Unable to load mixer: %s\n", snd_strerror(err));
      snd_mixer_close (handle);
      return -1;
    }

    snd_mixer_selem_id_set_index (sid, 0);
    /* the assumption being that there are not more than 10 cards present */
    snd_mixer_selem_id_set_name (sid, volsink);
    elem = snd_mixer_find_selem (handle, sid);
    if (!elem) {
      free (cardid);
      fprintf (stderr, "Unable to find control %s\n", volsink);
      snd_mixer_close (handle);
      return -1;
    }

    snd_mixer_selem_get_playback_volume_range (elem, &min, &max);
    if (action == VOL_SET) {
      lvol = (int) round ((double) *vol * (double) max / 100.0);
      snd_mixer_selem_set_playback_volume_all (elem, lvol);
    }
    snd_mixer_selem_get_playback_volume (elem, SND_MIXER_SCHN_FRONT_LEFT, &lvol);
    *vol = (int) round ((double) lvol / (double) max * 100.0);
    snd_mixer_close (handle);
    free (cardid);
  }

  return 0;
}

#endif /* hdr_alsa_asoundlib and not hd_pulse_pulseaudio */
