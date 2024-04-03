/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VOLUME
#define INC_VOLUME

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

#include "ilist.h"

typedef struct volsinklist volsinklist_t;

#define VOL_DEFAULT_NAME "default"

typedef enum {
  VOL_CHK_SINK,
  VOL_GET,
  VOL_SET,
  VOL_HAVE_SINK_LIST,
  VOL_GETSINKLIST,
  VOL_SET_SYSTEM_DFLT,
} volaction_t;

typedef struct volume volume_t;

volume_t  *volumeInit (const char *volpkg);
void      volumeFree (volume_t *volume);
bool      volumeHaveSinkList (volume_t *volume);
void      volumeSinklistInit (volsinklist_t *sinklist);
int       volumeCheckSink (volume_t *volume, const char *sinkname);
int       volumeGet (volume_t *volume, const char *sinkname);
int       volumeSet (volume_t *volume, const char *sinkname, int vol);
int       volumeGetSinkList (volume_t *volume, const char *sinkname, volsinklist_t *sinklist);
void      volumeFreeSinkList (volsinklist_t *sinklist);
ilist_t   *volumeInterfaceList (void);
char      *volumeCheckInterface (const char *volintfc);

void  voliDesc (const char **ret, int max);
int   voliProcess (volaction_t action, const char *sinkname, int *vol, volsinklist_t *sinklist, void **udata);
void  voliDisconnect (void);
void  voliCleanup (void **udata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_VOLUME */
