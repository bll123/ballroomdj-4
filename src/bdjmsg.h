#ifndef INC_BDJMSG_H
#define INC_BDJMSG_H

typedef enum {
  ROUTE_MAIN,
  ROUTE_GUI,
  ROUTE_PLAYER,
  ROUTE_MAX
} bdjmsgroute_t;

typedef enum {
  MSG_CLOSE_SOCKET,
  MSG_REQUEST_CLOSE,
  MSG_PLAYER_START,
  MSG_REQUEST_EXIT,     // standard shutdown
  MSG_FORCE_EXIT,       // only for testing
  MSG_MAX
} bdjmsgmsg_t;

#define BDJMSG_MAX_ARGS     1024
#define BDJMSG_MAX          (8 * 2 + 3 + BDJMSG_MAX_ARGS)

size_t msgEncode (long route, long msg, char *args, char *msgbuff, size_t mlen);
void msgDecode (char *msgbuff, long *route, long *msg, char *args);

#endif /* INC_BDJMSG_H */
