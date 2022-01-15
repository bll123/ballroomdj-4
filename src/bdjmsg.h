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
  MSG_PLAYER_VOLSINK_SET,   // to player: set volume sink
  MSG_PLAYBACK_BEGIN,       // to main: player has started playing a song
  MSG_PLAYBACK_FINISH,      // to main: player has finished playing song
                            // args: song fname
  MSG_PLAYLIST_QUEUE,       // args: playlist name
  MSG_SOCKET_CLOSE,
  MSG_SONG_PLAY,            // args: song fname
  MSG_SONG_PREP,            // args: song fname, duration, song-start
                            //    song-end, volume-adjustment-perc, gap
  MSG_MAX
} bdjmsgmsg_t;

typedef enum {
  PREP_SONG,
  PREP_ANNOUNCE,
} bdjmsgprep_t;

#define BDJMSG_MAX_ARGS     1024
#define BDJMSG_MAX          (8 * 2 + 3 + BDJMSG_MAX_ARGS)

#define MSG_ARGS_RS         0x1E
#define MSG_ARGS_RS_STR     "\x1E"

size_t    msgEncode (bdjmsgroute_t routefrom, bdjmsgroute_t route,
              bdjmsgmsg_t msg, char *args, char *msgbuff, size_t mlen);
void      msgDecode (char *msgbuff, bdjmsgroute_t *routefrom,
              bdjmsgroute_t *route, bdjmsgmsg_t *msg, char *args, size_t alen);

#endif /* INC_BDJMSG_H */
