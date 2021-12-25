#ifndef INC_VLCI_H
#define INC_VLCI_H

#include <vlc/vlc.h>
#include <vlc/libvlc_version.h>

#include "config.h"
#include "list.h"

typedef struct {
  libvlc_instance_t     *inst;
  char                  version [40];
  libvlc_media_player_t *mp;
  libvlc_state_t        state;
  int                   argc;
  char                  **argv;
  char                  *device;
  int                   initialvolume;
} vlcData_t;

double            vlcGetDuration (vlcData_t *vlcData);
double            vlcGetTime (vlcData_t *vlcData);
int               vlcIsPlaying (vlcData_t *vlcData);
int               vlcIsPaused (vlcData_t *vlcData);
int               vlcStop (vlcData_t *vlcData);
int               vlcPause (vlcData_t *vlcData);
int               vlcPlay (vlcData_t *vlcData);
double            vlcRate (vlcData_t *vlcData, double drate);
double            vlcSeek (vlcData_t *vlcData, double dpos);
int               vlcHaveAudioDevList (void);
int               vlcAudioDevSet (vlcData_t *vlcData, char *dev);
#if _lib_libvlc_audio_output_device_enum
list_t *          vlcAudioDevList (vlcData_t *vlcData);
#endif
char *            vlcVersion (vlcData_t *vlcData);
const char *      vlcState (vlcData_t *vlcData);
void              noInitialVolume (vlcData_t *vlcData);
int               vlcMedia (vlcData_t *vlcdata, char *fn);
int               vlcInit (vlcData_t *vlcData, int vlcargc, char *vlcargv[]);
void              vlcClose (vlcData_t *vlcData);
void              vlcRelease (vlcData_t *vlcData);
void              vlcExitHandler (vlcData_t *vlcData);
void              vlcEventHandler (const struct libvlc_event_t *event, void *);



#endif /* INC_VLCI_H */
