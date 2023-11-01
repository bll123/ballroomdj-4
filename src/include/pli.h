/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_PLI_H
#define INC_PLI_H

#include "slist.h"
#include "tmutil.h"
#include "volsink.h"

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
  PLI_STATE_ENDED,
  PLI_STATE_ERROR,
  PLI_STATE_MAX,
} plistate_t;

enum {
  PLI_SUPPORT_NONE    = 0x0000,
  PLI_SUPPORT_SEEK    = 0x0001,
  PLI_SUPPORT_SPEED   = 0x0002,
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

pli_t         *pliInit (const char *volpkg, const char *sinkname);
void          pliFree (pli_t *pli);
void          pliMediaSetup (pli_t *pli, const char *mediaPath);
void          pliStartPlayback (pli_t *pli, ssize_t dpos, ssize_t speed);
void          pliPause (pli_t *pli);
void          pliPlay (pli_t *pli);
void          pliStop (pli_t *pli);
ssize_t       pliSeek (pli_t *pli, ssize_t dpos);
ssize_t       pliRate (pli_t *pli, ssize_t drate);
void          pliClose (pli_t *pli);
ssize_t       pliGetDuration (pli_t *pli);
ssize_t       pliGetTime (pli_t *pli);
plistate_t    pliState (pli_t *pli);
int           pliSetAudioDevice (pli_t *pli, const char *dev);
int           pliAudioDeviceList (pli_t *pli, volsinklist_t *sinklist);
int           pliSupported (pli_t *pli);
const char    *pliStateText (pli_t *pli);
slist_t       *pliInterfaceList (void);

plidata_t     *pliiInit (const char *volpkg, const char *sinkname);
void          pliiFree (plidata_t *pliData);
void          pliiMediaSetup (plidata_t *pliData, const char *mediaPath);
void          pliiStartPlayback (plidata_t *pliData, ssize_t dpos, ssize_t speed);
void          pliiClose (plidata_t *pliData);
void          pliiPause (plidata_t *pliData);
void          pliiPlay (plidata_t *pliData);
void          pliiStop (plidata_t *pliData);
ssize_t       pliiSeek (plidata_t *pliData, ssize_t dpos);
ssize_t       pliiRate (plidata_t *pliData, ssize_t drate);
ssize_t       pliiGetDuration (plidata_t *pliData);
ssize_t       pliiGetTime (plidata_t *pliData);
plistate_t    pliiState (plidata_t *pliData);
int           pliiSetAudioDevice (plidata_t *pliData, const char *dev);
int           pliiAudioDeviceList (plidata_t *pliData, volsinklist_t *);
int           pliiSupported (plidata_t *pliData);
const char    *pliiDesc (void);

#endif
