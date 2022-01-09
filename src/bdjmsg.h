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
  MSG_HANDSHAKE,
  MSG_SET_DEBUG_LVL,
  MSG_EXIT_FORCE,           // only for testing, may not work yet.
  MSG_EXIT_REQUEST,         // standard shutdown
  MSG_PLAY_FADE,            // to main: passed on to player.
  MSG_PLAY_PAUSE,           // to main: passed on to player.
  MSG_PLAY_PLAY,            // to main:
                            //    starts playback, passed on to player.
  MSG_PLAY_STOP,            // to main: passed on to player.
//  MSG_PLAYER_STATUS_Q,      // query: to player
//  MSG_PLAYER_VOLSINK_Q,     // query: get list of volume sinks
  MSG_PLAYER_VOLSINK_SET,   // to player: set volume sink
  MSG_PLAYER_VOLUME_Q,      // query: to player
  MSG_PLAYER_VOLUME_SET,    // arguments: volume
  MSG_PLAYLIST_QUEUE,       // arguments: playlist name
  MSG_PLAYLIST_MAXTIME,     // arguments: playlist's max time
  MSG_PLAYBACK_BEGIN,       // to main: player has started playing a song
  MSG_PLAYBACK_FINISH,      // to main: player has finished playing song
                            // args: song fname
//  MSG_PLQ_LENGTH_Q,         // query: playlist queue length
//  MSG_PLQ_CONTENTS_Q,       // query: playlist queue contents
//  MSG_MUSICQ_LENGTH_Q,      // query: musicq length
//  MSG_MUSICQ_CONTENTS_Q,    // query: musicq contents
  MSG_SOCKET_CLOSE,
  MSG_SONG_PLAY,            // arguments: song fname
  MSG_SONG_PREP,            // arguments: song fname, duration
  MSG_MAX
} bdjmsgmsg_t;

#define BDJMSG_MAX_ARGS     1024
#define BDJMSG_MAX          (8 * 2 + 3 + BDJMSG_MAX_ARGS)

#define MSG_ARGS_RS         0x1E
#define MSG_ARGS_RS_STR     "\x1E"

size_t    msgEncode (long routefrom, long route, long msg,
              char *args, char *msgbuff, size_t mlen);
void      msgDecode (char *msgbuff, long *routefrom, long *route, long *msg,
              char *args, size_t alen);

#endif /* INC_BDJMSG_H */
