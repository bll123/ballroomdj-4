#ifndef INC_DBUSI_H
#define INC_DBUSI_H

typedef struct dbus dbus_t;

dbus_t * dbusConnInit (void);
void dbusConnClose (dbus_t *dbus);
void dbusMessageInit (dbus_t *dbus);
void dbusMessageSetData (dbus_t *dbus, const char *sdata, ...);
void dbusMessage (dbus_t *dbus, const char *bus, const char *objpath, const char *intfc, const char *method);

#endif /* INC_DBUSI_H */
