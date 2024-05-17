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
void dbusMessageSetDataArray (dbus_t *dbus, const char *sdata, ...);
void dbusMessageSetDataString (dbus_t *dbus, const char *sdata, ...);
bool dbusMessage (dbus_t *dbus, const char *bus, const char *objpath, const char *intfc, const char *method);
bool dbusResultGet (dbus_t *dbus, ...);
void dbusSetIntrospectionData (dbus_t *dbus, const char *intropection_xml);
int   dbusRegisterObject (dbus_t *dbus, const char *objpath, const char *intfc);
void dbusUnregisterObject (dbus_t *dbus, int intfcid);
void dbusSetCallbacks (dbus_t *dbus, void *udata, dbusCBmethod_t cbmethod, dbusCBpropget_t cbpropget);

#endif /* INC_DBUSI_H */
