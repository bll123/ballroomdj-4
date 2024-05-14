#ifndef INC_DBUSI_H
#define INC_DBUSI_H

#include <stdbool.h>

typedef struct dbus dbus_t;

enum {
  DBUS_NAME_WAIT,
  DBUS_NAME_CLOSED,
  DBUS_NAME_OPEN,
};

enum {
  DBUS_MAX_TRACKID = 200,
};

dbus_t * dbusConnInit (void);
void dbusConnectAcquireName (dbus_t *dbus, const char *instname, const char *intfc);
int   dbusCheckAcquireName (dbus_t *dbus);
void dbusConnClose (dbus_t *dbus);
void dbusMessageInit (dbus_t *dbus);
void *dbusMessageBuild (const char *sdata, ...);
void *dbusMessageBuildObj (const char *path);
void dbusMessageSetData (dbus_t *dbus, const char *sdata, ...);
void dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...);
bool dbusMessage (dbus_t *dbus, const char *bus, const char *objpath, const char *intfc, const char *method);
bool dbusResultGet (dbus_t *dbus, ...);
void dbusSetIntrospectionData (dbus_t *dbus, const char *intropection_xml);
int   dbusRegisterObject (dbus_t *dbus, const char *objpath, const char *intfc, void *udata);
void dbusUnregisterObject (dbus_t *dbus, int intfcid);

#endif /* INC_DBUSI_H */
