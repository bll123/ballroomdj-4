#ifndef INC_BDJMSG_H
#define INC_BDJMSG_H

typedef enum {
  ROUTE_NONE,       // anonymous
  ROUTE_MAIN,
  ROUTE_GUI,
  ROUTE_PLAYER,
  ROUTE_MAX
} bdjmsgroute_t;

typedef enum {
  MSG_NONE,
  MSG_EXIT_FORCE,           // only for testing, may not work yet.
  MSG_EXIT_REQUEST,         // standard shutdown
  MSG_PLAYER_PAUSE,         // to main
  MSG_PLAYER_PLAY,          // to main
  MSG_PLAYER_START,         // goes to main, not player
  MSG_PLAYER_STOP,          // to main
  MSG_PLAYLIST_LOAD,        // arguments: playlist name
  MSG_SOCKET_CLOSE,
  MSG_SONG_PLAY,            // arguments: path to song
  MSG_SONG_PREP,            // arguments: path to song
  MSG_MAX
} bdjmsgmsg_t;

#define BDJMSG_MAX_ARGS     1024
#define BDJMSG_MAX          (8 * 2 + 3 + BDJMSG_MAX_ARGS)

size_t msgEncode (long route, long msg, char *args, char *msgbuff, size_t mlen);
void msgDecode (char *msgbuff, long *route, long *msg, char *args);

#endif /* INC_BDJMSG_H */
