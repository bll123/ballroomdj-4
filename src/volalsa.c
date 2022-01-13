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
#include "configalsa.h"

#if _hdr_alsa_asoundlib

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>
#include <math.h>
#include <regex.h>
#include <alsa/asoundlib.h>

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
  char                  card [65];
  int                   err;

  snd_mixer_selem_id_alloca (&sid);

  if (action == VOL_GETSINKLIST) {
    /* regular expressions */
    regex_t               regex;
    int                   rc;
    regmatch_t            matches[3];
#define MATCHSIZE (sizeof(matches)/sizeof(regmatch_t))
    /* asound: devices */
    void                  **hints;
    void                  **n;
    const char            *filter;
    char                  *name;
    char                  *descr;
    char                  *io;
    int                   clidx;
    /* asound: cards */
    const char            *cid;
    const char            *mname;
    int                   cnumber;
    int                   idx;
    snd_ctl_card_info_t   *info;
    snd_ctl_t             *ctl;
    cardlist_t            cardlist [50];
    int                   cardcount;

    rc = regcomp(&regex, ":CARD=([A-Za-z0-9_-]+)", REG_EXTENDED);
    if (rc != 0) {
      fprintf (stderr, "Could not compile regex\n");
      return -1;
    }

    snd_ctl_card_info_alloca (&info);
    cnumber = -1;
    cardcount = 0;
    for (;;) {
      if ((err = snd_card_next (&cnumber)) < 0) {
        break;
      }
      if (cnumber < 0) {
        break;
      }
      sprintf (card, "hw:%d", cnumber);
      if ((err = snd_ctl_open (&ctl, card, 0)) < 0) {
        return -1;
      }
      err = snd_ctl_card_info(ctl, info);
      snd_ctl_close(ctl);
      if (err < 0) {
        return -1;
      }
      cid = snd_ctl_card_info_get_id (info);

      if ((err = snd_mixer_open (&handle, 0)) < 0) {
        fprintf (stderr, "Unable to open mixer: %s\n", snd_strerror(err));
        return -1;
      }
      if ((err = snd_mixer_attach (handle, card)) < 0) {
        snd_mixer_close (handle);
        return -1;
      }
      if ((err = snd_mixer_selem_register (handle, NULL, NULL)) < 0) {
        snd_mixer_close (handle);
        return -1;
      }
      if ((err = snd_mixer_load (handle)) < 0) {
        snd_mixer_close (handle);
        return -1;
      }

      elem = snd_mixer_first_elem(handle);
      while (elem != (snd_mixer_elem_t *) NULL) {
        snd_mixer_selem_get_id (elem, sid);
        mname = snd_mixer_selem_id_get_name (sid);
        if (! snd_mixer_selem_is_active (elem) ||
            (! snd_mixer_selem_has_playback_volume (elem) &&
             ! snd_mixer_selem_has_playback_volume_joined (elem) &&
             ! snd_mixer_selem_has_playback_switch (elem))) {
          elem = snd_mixer_elem_next (elem);
          continue;
        }
        cardlist[cardcount].cardnumber = cnumber;
        cardlist[cardcount].id = strdup (cid);
        assert (cardlist [cardcount].id != NULL);
        cardlist[cardcount].name = strdup (mname);
        assert (cardlist [cardcount].name != NULL);
        ++cardcount;
        elem = snd_mixer_elem_next (elem);
      }

      snd_mixer_close (handle);
    }

    /* devices */

    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
      return -1;
    }
    n = hints;
    filter = "Output";
    clidx = 0;
    while (*n != NULL) {
      io = snd_device_name_get_hint(*n, "IOID");
      name = snd_device_name_get_hint(*n, "NAME");
      if (io != NULL && strcmp(io, filter) != 0) {
        /* only output devices */
        goto skip;
      }

      rc = regexec (&regex, name, MATCHSIZE, matches, 0);
      /* rc=0 - ok ; rc=REG_NOMATCH ; otherwise error */

      descr = snd_device_name_get_hint(*n, "DESC");
      if (rc == 0) {
        for (int i = 0; i < cardcount; ++i) {
          if (strncmp (cardlist[i].id, name + matches[1].rm_so,
              matches[1].rm_eo - matches[1].rm_so) == 0) {
            clidx = i;
            break;
          }
        }
      }

      idx = sinklist->count;
      ++sinklist->count;
      sinklist->sinklist = realloc (sinklist->sinklist,
          sinklist->count * sizeof (volsinkitem_t));
      sinklist->sinklist [idx].defaultFlag = 0;
      sinklist->sinklist [idx].idxNumber = idx;
      sinklist->sinklist [idx].name = strdup (name);
      sinklist->sinklist [idx].description = strdup (descr);
      // cardlist [clidx]  .id and .name are not used.

      if (descr != NULL) {
        free(descr);
      }
skip:
      if (name != NULL) {
        free (name);
      }
      if (io != NULL) {
        free (io);
      }
      n++;
    }

    for (int i = 0; i < cardcount; ++i) {
      free (cardlist[i].id);
      free (cardlist[i].name);
    }

    snd_device_name_free_hint(hints);
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
    sprintf (card, "hw:%s", cardid);
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
