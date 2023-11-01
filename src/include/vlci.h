/*
 * Copyright 2021-2023 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_VLCI_H
#define INC_VLCI_H

#include "config.h"

/* winsock2.h should come before windows.h */
#if _hdr_winsock2
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-declarations"
# include <winsock2.h>
# pragma clang diagnostic pop
#endif
#if _hdr_windows
# include <windows.h>
#endif

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "pli.h"
#include "volsink.h"

typedef struct vlcData vlcData_t;

ssize_t           vlcGetDuration (vlcData_t *vlcData);
ssize_t           vlcGetTime (vlcData_t *vlcData);
int               vlcIsPlaying (vlcData_t *vlcData);
int               vlcIsPaused (vlcData_t *vlcData);
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

#endif /* INC_VLCI_H */
