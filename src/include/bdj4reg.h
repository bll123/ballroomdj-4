#ifndef INC_BDJ4REG_H
#define INC_BDJ4REG_H

#include "conn.h"

void regStart (void);
void regRegister (conn_t *conn);
void regRegisterExit (conn_t *conn);
void regQuery (conn_t *conn);

#endif /* INC_BDJ4REG_H */
