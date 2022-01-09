#ifndef INC_PLI_H
#define INC_PLI_H

#include "vlci.h"

typedef enum {
  PLI_CMD_STATUS_WAIT,
  PLI_CMD_STATUS_OK,
} plicmdstatus_t;

typedef enum {
  PLI_STATE_NONE,
  PLI_STATE_OPENING,
  PLI_STATE_BUFFERING,
  PLI_STATE_PLAYING,
  PLI_STATE_PAUSED,
  PLI_STATE_STOPPED,
  PLI_STATE_ENDED,
  PLI_STATE_ERROR,
} plistate_t;

typedef struct {
  void              *plData;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
} plidata_t;

plidata_t     *pliInit (void);
void          pliFree (plidata_t *pliData);
void          pliMediaSetup (plidata_t *pliData, char *mediaPath);
void          pliStartPlayback (plidata_t *pliData);
void          pliClose (plidata_t *pliData);
void          pliPause (plidata_t *pliData);
void          pliPlay (plidata_t *pliData);
void          pliStop (plidata_t *pliData);
ssize_t       pliGetDuration (plidata_t *pliData);
ssize_t       pliGetTime (plidata_t *pliData);
plistate_t    pliState (plidata_t *pliData);

#endif
