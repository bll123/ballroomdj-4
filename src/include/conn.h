/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#ifndef INC_CONN_H
#define INC_CONN_H

#include <stdint.h>
#include <stdbool.h>

#include "nodiscard.h"
#include "sock.h"
#include "bdjmsg.h"

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

typedef struct conn conn_t;

/**
 * Initialize a connection.
 *
 * bdjvars must be initialized before initializing a connection.
 *
 * @param[in] routefrom The route that is sending messages.
 * @return conn_t array.
 */
NODISCARD conn_t    * connInit (bdjmsgroute_t routefrom);

/**
 * Free a connection.
 *
 * All connections are invalid afterwards.
 *
 * @param[in] conn The connection type returned from connInit().
 */
void      connFree (conn_t *conn);

/**
 * Get the port number used for a connection.
 * @param[in] conn The connection.
 * @return port number
 */
uint16_t  connPort (conn_t *conn, bdjmsgroute_t route);

/**
 * Connect to a route.
 *
 * Sends a handshake message.
 *
 * @param[in] conn The connection.
 * @param[in] route The route number to connect to.
 */
void      connConnect (conn_t *conn, bdjmsgroute_t route);

/**
 * Disconnect from a route.
 *
 * @param[in] conn The connection.
 * @param[in] route The route number to connect to.
 */
void      connDisconnect (conn_t *conn, bdjmsgroute_t route);

/**
 * Disconnect from all connections.
 *
 * @param[in] conn The connection.
 */
void      connDisconnectAll (conn_t *conn);

/**
 * Called on receipt of a handshake message to update the connection.
 *
 * @param[in] conn The connection.
 * @param[in] routefrom The route sending the handshake.
 */
void      connProcessHandshake (conn_t *conn, bdjmsgroute_t routefrom);

/**
 * Check for received handshakes w/o outgoing connections and makes
 * the connection to finish the full handshake protocol.
 *
 * @param[in] conn The connection.
 */
void      connProcessUnconnected (conn_t *conn);

/**
 * Send a message.
 *
 * @param[in] conn The connection.
 * @param[in] route The route to send the message to.
 * @param[in] msg The message.
 * @param[in] args Any arguments to the message or NULL.
 */
void      connSendMessage (conn_t *conn, bdjmsgroute_t route,
              bdjmsgmsg_t msg, const char *args);

/**
 * Check and see if a connection is active.
 *
 * @param[in] conn The connection.
 * @param[in] route The outgoing route.
 * @return true if connected.
 */
bool      connIsConnected (conn_t *conn, bdjmsgroute_t route);

/**
 * Check and see if a handshake has been received.
 *
 * @param[in] conn The connection.
 * @param[in] route The outgoing route.
 * @return true if a handshake has been received.
 */
bool      connHaveHandshake (conn_t *conn, bdjmsgroute_t route);

bool      connWaitClosed (conn_t *conn, int *stopwaitcount);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

#endif /* INC_CONN_H */

