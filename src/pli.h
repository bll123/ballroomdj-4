#ifndef INC_PLI_H
#define INC_PLI_H

#include "vlci.h"

typedef struct {
  void        *plData;
} plidata_t;

plidata_t     *pliInit (void);
void          pliFree (plidata_t *pliData);
void          pliMediaSetup (plidata_t *pliData, char *afname);
void          pliStartPlayback (plidata_t *pliData);
void          pliClose (plidata_t *pliData);
void          pliPause (plidata_t *pliData);
void          pliPlay (plidata_t *pliData);
void          pliStop (plidata_t *pliData);

#endif
