/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VLCI_H
#define INC_VLCI_H

#include "config.h"

#if __has_include (<vlc/vlc.h>)

# include <vlc/vlc.h>
# include <vlc/libvlc_version.h>

# include "pli.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct vlcdata vlcdata_t;

ssize_t           vlcGetDuration (vlcdata_t *vlcdata);
ssize_t           vlcGetTime (vlcdata_t *vlcdata);
int               vlcStop (vlcdata_t *vlcdata);
int               vlcPause (vlcdata_t *vlcdata);
int               vlcPlay (vlcdata_t *vlcdata);
ssize_t           vlcSeek (vlcdata_t *vlcdata, ssize_t dpos);
double            vlcRate (vlcdata_t *vlcdata, double drate);
const char *      vlcVersion (vlcdata_t *vlcdata);
plistate_t        vlcState (vlcdata_t *vlcdata);
int               vlcMedia (vlcdata_t *vlcdata, const char *fn, int sourceType);
vlcdata_t *       vlcInit (int vlcargc, char *vlcargv [], char *vlcopt []);
void              vlcClose (vlcdata_t *vlcdata);
void              vlcRelease (vlcdata_t *vlcdata);
int               vlcSetAudioDev (vlcdata_t *vlcdata, const char *dev, int plidevtype);
bool              vlcVersionLinkCheck (void);
bool              vlcVersionCheck (void);
int               vlcGetVolume (vlcdata_t *vlcdata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* vlc/vlc.h */

#endif /* INC_VLCI_H */
