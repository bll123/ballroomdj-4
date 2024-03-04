/*
 * Copyright 2021-2024 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VLCI_H
#define INC_VLCI_H

#include "config.h"

#if _hdr_vlc_vlc

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "pli.h"
#include "volsink.h"

typedef struct vlcData vlcData_t;

ssize_t           vlcGetDuration (vlcData_t *vlcData);
ssize_t           vlcGetTime (vlcData_t *vlcData);
int               vlcStop (vlcData_t *vlcData);
int               vlcPause (vlcData_t *vlcData);
int               vlcPlay (vlcData_t *vlcData);
ssize_t           vlcSeek (vlcData_t *vlcData, ssize_t dpos);
double            vlcRate (vlcData_t *vlcData, double drate);
bool              vlcHaveAudioDevList (void);
int               vlcAudioDevSet (vlcData_t *vlcData, const char *dev);
#if _lib_libvlc_audio_output_device_enum
int               vlcAudioDevList (vlcData_t *vlcData, volsinklist_t *sinklist);
#endif
const char *      vlcVersion (vlcData_t *vlcData);
plistate_t        vlcState (vlcData_t *vlcData);
int               vlcMedia (vlcData_t *vlcdata, const char *fn);
vlcData_t *       vlcInit (int vlcargc, char *vlcargv [], char *vlcopt []);
void              vlcClose (vlcData_t *vlcData);
void              vlcRelease (vlcData_t *vlcData);

#endif /* _hdr_vlc_vlc */

#endif /* INC_VLCI_H */
