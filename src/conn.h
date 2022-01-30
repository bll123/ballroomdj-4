#ifndef INC_CONN_H
#define INC_CONN_H

#include <stdint.h>
#include <stdbool.h>

#include "sock.h"
#include "bdjmsg.h"

typedef struct {
  Sock_t        sock;
  uint16_t      port;
  bdjmsgroute_t routefrom;
  bool          handshake : 1;
  bool          connected : 1;
} conn_t;

conn_t    * connInit (bdjmsgroute_t routefrom);
void      connFree (conn_t *conn);
void      connConnect (conn_t *conn, bdjmsgroute_t route);
void      connDisconnect (conn_t *conn, bdjmsgroute_t route);
void      connDisconnectAll (conn_t *conn);
void      connReconnect (conn_t *conn, bdjmsgroute_t route);
void      connProcessHandshake (conn_t *conn, bdjmsgroute_t routefrom);
void      connSendMessage (conn_t *conn, bdjmsgroute_t route,
              bdjmsgmsg_t msg, char *args);
void      connConnectResponse (conn_t *conn, bdjmsgroute_t route);
void      connClearHandshake (conn_t *conn, bdjmsgroute_t route);
bool      connIsConnected (conn_t *conn, bdjmsgroute_t route);
bool      connHaveHandshake (conn_t *conn, bdjmsgroute_t route);

#endif /* INC_CONN_H */

