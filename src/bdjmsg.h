#ifndef INC_BDJMSG_H
#define INC_BDJMSG_H

typedef enum {
  ROUTE_NONE,       // anonymous
  ROUTE_MAIN,
  ROUTE_GUI,
  ROUTE_PLAYER,
  ROUTE_CLICOMM,
  ROUTE_MOBILEMQ,
  ROUTE_REMCTRL,
  ROUTE_MAX
} bdjmsgroute_t;

typedef enum {
  MSG_NULL,
  MSG_HANDSHAKE,
  MSG_SET_DEBUG_LVL,
  MSG_EXIT_FORCE,           // only for testing, may not work yet.
  MSG_EXIT_REQUEST,         // standard shutdown
  MSG_PLAY_PAUSE,           // to player
  MSG_PLAY_PLAY,            // to main:
                            //    starts playback, passed on to player.
  MSG_PLAY_PLAYPAUSE,       // to player:
  MSG_PLAY_FADE,            // to player.
  MSG_PLAY_PAUSEATEND,      // to player: toggle
  MSG_PLAY_STOP,            // to player.
  MSG_PLAY_NEXTSONG,        // to player.
  MSG_PLAY_REPEAT,          // to player. toggle
  MSG_PLAY_RATE,            // to player. args: rate as percentage.
  MSG_PLAYER_VOL_MUTE,      // to player. toggle.
  MSG_PLAYER_VOLUME,        // to player. args: volume as percentage.
  MSG_PLAYER_VOLSINK_SET,   // to player: set volume sink
  MSG_PLAYBACK_STOP,        // to main: player has stopped playing song
                            //    do not continue.
  MSG_PLAYBACK_FINISH,      // to main: player has finished playing song
                            // args: song fname
  MSG_PLAYLIST_QUEUE,       // args: playlist name
  MSG_PLAY_PAUSEATEND_STATE,// args: 0/1
  MSG_PLAYER_STATE,         // args: player state
  MSG_SOCKET_CLOSE,
  MSG_SONG_PLAY,            // args: song fname
  MSG_SONG_PREP,            // args: song fname, duration, song-start
                            //    song-end, volume-adjustment-perc, gap
  MSG_MARQUEE_DATA,         // args: mq data
  MSG_GET_DANCE_LIST,       //
  MSG_DANCE_LIST_DATA,      // args: html option list
  MSG_GET_PLAYLIST_LIST,    //
  MSG_PLAYLIST_LIST_DATA,   // args: html option list
  MSG_GET_STATUS,           // get main/player status
  MSG_PLAYER_STATUS_DATA,   // response to main
  MSG_STATUS_DATA,          // response to remote control
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
