/*
 * Copyright 2021-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "dylib.h"
#include "ilist.h"
#include "tmutil.h"
#include "volsink.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef enum {
  PLI_CMD_STATUS_WAIT,
  PLI_CMD_STATUS_OK,
} plicmdstatus_t;

typedef enum {
  PLI_STATE_NONE,
  PLI_STATE_IDLE,
  PLI_STATE_OPENING,
  PLI_STATE_BUFFERING,
  PLI_STATE_PLAYING,
  PLI_STATE_PAUSED,
  PLI_STATE_STOPPED,
  PLI_STATE_STOPPING,
  PLI_STATE_ERROR,
  PLI_STATE_MAX,
} plistate_t;

enum {
  PLI_SUPPORT_NONE        = 0,
  PLI_SUPPORT_SEEK        = (1 << 0),
  PLI_SUPPORT_SPEED       = (1 << 1),
  PLI_SUPPORT_DEVLIST     = (1 << 2),
  PLI_SUPPORT_CROSSFADE   = (1 << 3),
  PLI_SUPPORT_STREAM      = (1 << 4),
  PLI_SUPPORT_STREAM_SPD  = (1 << 5),
};

typedef enum {
  PLI_DEFAULT_DEV,
  PLI_SELECTED_DEV,
} plidev_t;

enum {
  PLI_MAX_SOURCE = 2,
  PLI_MAX_UNSUPPORTED = 5,    /* at this time, macosavi has 3 */
};

static inline bool
pliCheckSupport (int supported, int supportflag)
{
  bool  rc;

  rc = (supported & supportflag) == supportflag;
  return rc;
}

typedef struct plidata plidata_t;

typedef struct pli pli_t;

pli_t         *pliInit (const char *plipkg, const char *plinm);
void          pliFree (pli_t *pli);
void          pliMediaSetup (pli_t *pli, const char *mediaPath, const char *fullMediaPath, int sourceType);
void          pliStartPlayback (pli_t *pli, ssize_t dpos, ssize_t speed);
void          pliCrossFade (pli_t *pli, const char *mediaPath, const char *fullMediaPath, int sourceType);
void          pliCrossFadeVolume (pli_t *pli, int vol);
void          pliPause (pli_t *pli);
void          pliPlay (pli_t *pli);
void          pliStop (pli_t *pli);
ssize_t       pliSeek (pli_t *pli, ssize_t dpos);
ssize_t       pliRate (pli_t *pli, ssize_t drate);
void          pliClose (pli_t *pli);
ssize_t       pliGetDuration (pli_t *pli);
ssize_t       pliGetTime (pli_t *pli);
plistate_t    pliState (pli_t *pli);
int           pliSetAudioDevice (pli_t *pli, const char *dev, plidev_t plidevtype);
int           pliAudioDeviceList (pli_t *pli, volsinklist_t *sinklist);
int           pliSupported (pli_t *pli);
int           pliUnsupportedFileTypes (pli_t *pli, int types [], size_t typmax);

int           pliGetVolume (pli_t *pli);      // for debugging
const char    *pliStateText (pli_t *pli);
ilist_t       *pliInterfaceList (void);

plidata_t     *pliiInit (const char *plinm, const char *playerargs);
void          pliiFree (plidata_t *pliData);
void          pliiCleanup (void);
void          pliiMediaSetup (plidata_t *pliData, const char *mediaPath, const char *fullMediaPath, int sourceType);
void          pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed);
void          pliiCrossFade (plidata_t *plidata, const char *mediaPath, const char *fullMediaPath, int sourceType);
void          pliiCrossFadeVolume (plidata_t *plidata, int vol);
void          pliiClose (plidata_t *pliData);
void          pliiPause (plidata_t *pliData);
void          pliiPlay (plidata_t *pliData);
void          pliiStop (plidata_t *pliData);
ssize_t       pliiSeek (plidata_t *pliData, ssize_t dpos);
ssize_t       pliiRate (plidata_t *pliData, ssize_t drate);
ssize_t       pliiGetDuration (plidata_t *pliData);
ssize_t       pliiGetTime (plidata_t *pliData);
plistate_t    pliiState (plidata_t *pliData);
int           pliiSetAudioDevice (plidata_t *pliData, const char *dev, plidev_t plidevtype);
int           pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *);
int           pliiSupported (plidata_t *pliData);
void          pliiDesc (const char **ret, int max);
int           pliiGetVolume (plidata_t *pliData);
int           pliiUnsupportedFileTypes (plidata_t *pliData, int types [], size_t typmax);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

