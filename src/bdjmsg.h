#ifndef INC_BDJMSG_H
#define INC_BDJMSG_H

typedef enum {
  ROUTE_NONE,       // anonymous
  ROUTE_MAIN,
  ROUTE_GUI,
  ROUTE_PLAYER,
  ROUTE_CLICOMM,
  ROUTE_MAX
} bdjmsgroute_t;

typedef enum {
  MSG_NONE,
  MSG_ECHO,
  MSG_ECHO_RESP,
  MSG_SET_DEBUG_LVL,
  MSG_EXIT_FORCE,           // only for testing, may not work yet.
  MSG_EXIT_REQUEST,         // standard shutdown
  MSG_PLAYER_PAUSE,         // direct to player
  MSG_PLAYER_PLAY,          // direct to player
  MSG_PLAYER_STOP,          // direct to player
  MSG_PLAYER_STATUS_Q,      // query: to player
  MSG_PLAYER_VOLSINK_Q,     // query: get list of volume sinks
  MSG_PLAYER_VOLSINK_SET,   // to player: set volume sink
  MSG_PLAYER_VOLUME_Q,      // query: to player
  MSG_PLAYER_VOLUME_SET,    // arguments: volume
  MSG_PLAYLIST_QUEUE,       // arguments: playlist name
  MSG_PLQ_LENGTH_Q,         // query: playlist queue length
  MSG_PLQ_CONTENTS_Q,       // query: playlist queue contents
  MSG_MUSICQ_LENGTH_Q,      // query: musicq length
  MSG_MUSICQ_CONTENTS_Q,    // query: musicq contents
  MSG_SOCKET_CLOSE,
  MSG_SONG_PLAY,            // arguments: full path to song
  MSG_SONG_PREP,            // arguments: full path to song
  MSG_MAX
} bdjmsgmsg_t;

#define BDJMSG_MAX_ARGS     1024
#define BDJMSG_MAX          (8 * 2 + 3 + BDJMSG_MAX_ARGS)

size_t    msgEncode (long routefrom, long route, long msg,
              char *args, char *msgbuff, size_t mlen);
void      msgDecode (char *msgbuff, long *routefrom, long *route, long *msg,
              char *args, size_t alen);

#endif /* INC_BDJMSG_H */
