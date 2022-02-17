#ifndef INC_PLI_H
#define INC_PLI_H

#include "dylib.h"
#include "tmutil.h"

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
  char              *name;
  void              *plData;
  ssize_t           duration;
  ssize_t           playTime;
  plistate_t        state;
  mstime_t          playStart;                    // for the null player
} plidata_t;

typedef struct {
  dlhandle_t        *dlHandle;
  plidata_t         *(*pliiInit) (void);
  void              (*pliiFree) (plidata_t *);
  void              (*pliiMediaSetup) (plidata_t *, char *);
  void              (*pliiStartPlayback) (plidata_t *, ssize_t pos, ssize_t speed);
  void              (*pliiClose) (plidata_t *);
  void              (*pliiPause) (plidata_t *);
  void              (*pliiPlay) (plidata_t *);
  void              (*pliiStop) (plidata_t *);
  ssize_t           (*pliiSeek) (plidata_t *pliData, ssize_t pos);
  ssize_t           (*pliiRate) (plidata_t *pliData, ssize_t rate);
  ssize_t           (*pliiGetDuration) (plidata_t *);
  ssize_t           (*pliiGetTime) (plidata_t *);
  plistate_t        (*pliiState) (plidata_t *);
  plidata_t         *pliData;
} pli_t;

pli_t         *pliInit (void);
void          pliFree (pli_t *pli);
void          pliMediaSetup (pli_t *pli, char *mediaPath);
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

plidata_t     *pliiInit (void);
void          pliiFree (plidata_t *pliData);
void          pliiMediaSetup (plidata_t *pliData, char *mediaPath);
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

#endif
