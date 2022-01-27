#ifndef INC_BDJMSG_H
#define INC_BDJMSG_H

typedef enum {
  ROUTE_NONE,       // anonymous
  ROUTE_MAIN,
  ROUTE_PLAYERGUI,
  ROUTE_CONFIGGUI,
  ROUTE_MANAGEGUI,
  ROUTE_PLAYER,
  ROUTE_CLICOMM,
  ROUTE_MOBILEMQ,
  ROUTE_REMCTRL,
  ROUTE_MARQUEE,
  ROUTE_MAX
} bdjmsgroute_t;

typedef enum {
  MSG_NULL,
  MSG_HANDSHAKE,
  MSG_CONNECT_REQ,          // request a re-connect
  MSG_SOCKET_CLOSE,
  MSG_EXIT_REQUEST,         // standard shutdown
    /* to main */
  MSG_GET_STATUS,           // get main/player status
  MSG_MUSICQ_MOVE_DOWN,     // args: idx
  MSG_MUSICQ_MOVE_TOP,      // args: idx
  MSG_MUSICQ_MOVE_UP,       // args: idx
  MSG_MUSICQ_REMOVE,        // args: idx
  MSG_MUSICQ_TOGGLE_PAUSE,  //
  MSG_MUSICQ_TRUNCATE,      // args: idx
  MSG_MUSICQ_INSERT,        // args: idx, song name
  MSG_PLAYLIST_CLEARPLAY,   // args: playlist name
  MSG_PLAYLIST_QUEUE,       // args: playlist name
  MSG_PLAY_PLAY,            // always to main
                            //    starts playback, passed on to player.
  MSG_PLAY_PLAYPAUSE,       // always to main
  MSG_QUEUE_CLEAR,          //
  MSG_DANCE_QUEUE,          // args: dance idx
  MSG_DANCE_QUEUE5,         // args: dance idx
    /* to player */
  MSG_PLAYER_VOL_MUTE,      // to player. toggle.
  MSG_PLAYER_VOLSINK_SET,   // to player: set volume sink
  MSG_PLAYER_VOLUME,        // to player. args: volume as percentage.
  MSG_PLAY_FADE,            // to player.
  MSG_PLAY_NEXTSONG,        // to player.
  MSG_PLAY_PAUSEATEND,      // to player: toggle
  MSG_PLAY_PAUSE,           // to player
  MSG_PLAY_RATE,            // to player. args: rate as percentage.
  MSG_PLAY_REPEAT,          // to player. toggle
  MSG_PLAY_STOP,            // to player.
  MSG_SONG_PLAY,            // args: song fname
  MSG_SONG_PREP,            // args: song fname, duration, song-start
                            //    song-end, volume-adjustment-perc, gap
    /* from player */
  MSG_PLAY_PAUSEATEND_STATE,// args: 0/1
  MSG_PLAYBACK_BEGIN,       // to main: player has started playing the
                            //    song.  This message is sent after the
                            //    any announcement has finished.
  MSG_PLAYBACK_STOP,        // to main: player has stopped playing song
                            //    do not continue.
  MSG_PLAYBACK_FINISH,      // to main: player has finished playing song
                            //    args: song fname
  MSG_PLAYER_STATE,         // args: player state
  MSG_PLAYER_STATUS_DATA,   // response to get_status; to main
    /* to/from web servers */
  MSG_GET_DANCE_LIST,       //
  MSG_MARQUEE_DATA,         // args: mq json data
  MSG_DANCE_LIST_DATA,      // args: html option list
  MSG_GET_PLAYLIST_LIST,    //
  MSG_PLAYLIST_LIST_DATA,   // args: html option list
  MSG_STATUS_DATA,          // response to remote control
    /* to marquee */
  MSG_MARQUEE_SHOW,
  MSG_MAX,
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
