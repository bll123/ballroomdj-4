/*
 * Copyright 2023-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_DBUSI_H
#define INC_DBUSI_H

#include <stdbool.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct dbus dbus_t;

enum {
  DBUS_NAME_WAIT,
  DBUS_NAME_CLOSED,
  DBUS_NAME_OPEN,
};

enum {
  DBUS_MAX_TRACKID = 200,
};

typedef bool (*dbusCBmethod_t)(const char *intfc, const char *method, void *udata);
typedef bool (*dbusCBpropget_t)(const char *intfc, const char *property, void *udata);

dbus_t * dbusConnInit (void);
void dbusConnectAcquireName (dbus_t *dbus, const char *instname, const char *intfc);
int   dbusCheckAcquireName (dbus_t *dbus);
void dbusConnClose (dbus_t *dbus);
void dbusMessageInit (dbus_t *dbus);
void *dbusMessageBuild (const char *sdata, ...);
void *dbusMessageBuildObj (const char *path);
void dbusMessageSetData (dbus_t *dbus, const char *sdata, ...);
/* void dbusMessageSetDataTuple (dbus_t *dbus, const char *sdata, ...); */
void dbusMessageInitArray (dbus_t *dbus, const char *sdata);
void dbusMessageAppendArray (dbus_t *dbus, const char *sdata, ...);
void *dbusMessageFinalizeArray (dbus_t *dbus);
void dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...);
bool dbusMessage (dbus_t *dbus, const char *bus, const char *objpath, const char *intfc, const char *method);
bool dbusResultGet (dbus_t *dbus, ...);
void dbusSetInterfaceSkeleton (dbus_t *dbus, void *skel, const char *objpath);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_DBUSI_H */
