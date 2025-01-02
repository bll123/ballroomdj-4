/*
 * Copyright 2024-2025 Brad Lanam Pleasant Hill CA
 */

#ifndef INC_CONTROLLER_H
#define INC_CONTROLLER_H

#include <stdint.h>

#include "callback.h"
#include "ilist.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

enum {
  CONTROLLER_NONE,
  CONTROLLER_PLAY,
  CONTROLLER_PLAYPAUSE,
  CONTROLLER_PAUSE,
  CONTROLLER_STOP,
  CONTROLLER_NEXT,
  CONTROLLER_SEEK,      // long arg
  CONTROLLER_OPEN_URI,  // string arg
  CONTROLLER_REPEAT,
  CONTROLLER_RATE,
  CONTROLLER_QUIT,
};

enum {
  CONT_METADATA_ALBUM,
  CONT_METADATA_ALBUMARTIST,
  CONT_METADATA_ARTIST,
  CONT_METADATA_ART_URI,
  CONT_METADATA_DURATION,
  CONT_METADATA_GENRE,
  CONT_METADATA_SONGEND,
  CONT_METADATA_SONGSTART,
  CONT_METADATA_TITLE,
  CONT_METADATA_TRACKID,
  CONT_METADATA_URI,
  CONT_METADATA_MAX,
};

typedef struct controller controller_t;
typedef struct contdata contdata_t;

typedef struct {
  const char    *album;
  const char    *albumartist;
  const char    *artist;
  const char    *title;
  const char    *uri;
  const char    *arturi;
  const char    *genre;
  int32_t       songstart;
  int32_t       songend;
  int32_t       trackid;
  int32_t       duration;
} contmetadata_t;

const char *controllerDesc (void);
controller_t *controllerInit (const char *contpkg);
void controllerFree (controller_t *cont);
void controllerSetup (controller_t *cont);
bool controllerCheckReady (controller_t *cont);
void controllerSetCallbacks (controller_t *cont, callback_t *cb, callback_t *cburi);
void controllerSetPlayState (controller_t *cont, int state);
void controllerSetRepeatState (controller_t *cont, bool state);
void controllerSetPosition (controller_t *cont, int32_t pos);
void controllerSetRate (controller_t *cont, int rate);
void controllerSetVolume (controller_t *cont, int volume);
void controllerInitMetadata (contmetadata_t *cmetadata);
void controllerSetCurrent (controller_t *cont, contmetadata_t *cmetadata);
ilist_t *controllerInterfaceList (void);

void contiDesc (const char **ret, int max);
contdata_t *contiInit (const char *instname);
void contiFree (contdata_t *contdata);
void contiSetup (contdata_t *contdata);
bool contiCheckReady (contdata_t *contdata);
void contiSetCallbacks (contdata_t *contdata, callback_t *cb, callback_t *cburi);
void contiSetPlayState (contdata_t *contdata, int state);
void contiSetRepeatState (contdata_t *contdata, bool state);
void contiSetPosition (contdata_t *contdata, int32_t pos);
void contiSetRate (contdata_t *contdata, int rate);
void contiSetVolume (contdata_t *contdata, int volume);
void contiSetCurrent (contdata_t *contdata, contmetadata_t *cmetadata);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_CONTROLLER_H */
