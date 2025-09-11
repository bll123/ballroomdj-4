/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include <stdint.h>

#if defined (__cplusplus) || defined (c_plusplus)
extern "C" {
#endif

/* when a new route is added, update: */
/* conn.c : needs to know the port for each route */
/* bdjmsg.c: debugging information for the route */
/* bdjvars.h: port enum */
/* lock.c: lock name */
typedef enum {
  ROUTE_NONE,       // anonymous
  ROUTE_ALTINST,    // not used
  ROUTE_BPM_COUNTER,
  ROUTE_CONFIGUI,
  ROUTE_DBUPDATE,   // the main db update process
  ROUTE_HELPERUI,
  ROUTE_MAIN,
  ROUTE_MANAGEUI,
  ROUTE_MARQUEE,
  ROUTE_MOBILEMQ,
  ROUTE_PLAYER,
  ROUTE_PLAYERUI,
  ROUTE_REMCTRL,
  ROUTE_SERVER,
  ROUTE_STARTERUI,
  ROUTE_TEST_SUITE,
  ROUTE_PODCASTUPD,
  ROUTE_MAX,
} bdjmsgroute_t;

typedef enum {
  MSG_NULL,
  MSG_EXIT_REQUEST,         // standard shutdown
  MSG_HANDSHAKE,
  MSG_SOCKET_CLOSE,
  MSG_DB_RELOAD,            // sent by manageui to starterui,
                            // then sent by starterui to
                            // playerui, main, bdj4server.
  MSG_DB_LOADED,            // sent by main to manageui
                            // after the db update is complete
  MSG_DB_ENTRY_UPDATE,      // args: dbidx
  MSG_DB_ENTRY_REMOVE,      // args: dbidx
  MSG_DB_ENTRY_UNREMOVE,    // args: dbidx
  MSG_WINDOW_FIND,          // recover the window.

  /* to main */
  MSG_GET_STATUS,           // get main/player status
  MSG_MUSICQ_INSERT,        // args: music-q-idx, idx, song name
  MSG_MUSICQ_MOVE_DOWN,     // args: music-q-idx, idx
  MSG_MUSICQ_MOVE_TOP,      // args: music-q-idx, idx
  MSG_MUSICQ_MOVE_UP,       // args: music-q-idx, idx
  MSG_MUSICQ_REMOVE,        // args: music-q-idx, idx
  MSG_MUSICQ_SWAP,          // args: music-q-idx, idx-a, idx-b
  MSG_MUSICQ_SET_PLAYBACK,  // args: music queue for playback
  MSG_MUSICQ_SET_LEN,       // args: length
                            //    used to override the playerqlen in options
                            //    for the song list editor
  MSG_MUSICQ_TOGGLE_PAUSE,  // args: music-q-idx, idx
  MSG_MUSICQ_TRUNCATE,      // args: music-q-idx, start-idx
                            //    only truncates the music queue
  MSG_CMD_PLAY,             // always to main, starts playback
  MSG_CMD_PLAYPAUSE,        // always to main
  MSG_CMD_NEXTSONG_PLAY,    // to main.
                            // do a next-song if necessary and then play.
  MSG_QUEUE_CLEAR,          // args: music-q-idx
                            //    clears both the playlist queue and
                            //    all of the music queue.
  MSG_QUEUE_DANCE,          // args: music-q-idx, dance idx, count
  MSG_QUEUE_PLAYLIST,       // args: music-q-idx, playlist name, edit-flag
  MSG_QUEUE_SWITCH_EMPTY,   // args: true/false
  MSG_QUEUE_MIX,            // args: music-q-idx
  MSG_START_MARQUEE,
  MSG_STOP_MARQUEE,
  MSG_PL_OVERRIDE_STOP_TIME,  // args: stop-time
  MSG_PL_CLEAR_QUEUE,       // args: music-q-idx
                            // tells main to clear the playlist queue.
                            //   used by the song list editor.

  /* to player */
  MSG_PLAYER_VOL_MUTE,      // to player. toggle.
  MSG_PLAYER_VOLUME,        // to player. args: volume as percentage.
  MSG_PLAYER_SUPPORT,       // to/from player. query support/return support
  MSG_PLAY_FADE,            // to player.
  MSG_PLAY_NEXTSONG,        // to player.
  MSG_PLAY_PAUSEATEND,      // to player: toggle
  MSG_PLAY_PAUSE,           // to player
  MSG_PLAY_PLAY,            //
  MSG_PLAY_PLAYPAUSE,       //
  MSG_PLAY_REPEAT,          // to player. toggle
  MSG_PLAY_SEEK,            // to player. args: position
  MSG_PLAY_SONG_BEGIN,      // to player.
  MSG_PLAY_SPEED,           // to player. args: rate as percentage.
  MSG_PLAY_STOP,            // to player.
  MSG_PLAY_RESET_VOLUME,    // to player.
  MSG_SONG_PLAY,            // args: song fname
  MSG_SONG_PREP,            // args: song fname, duration, song-start
                            //    song-end, volume-adjustment-perc, ann-flag
  MSG_SONG_CLEAR_PREP,      // args: song fname
  MSG_SET_PLAYBACK_GAP,     // args: gap
  MSG_SET_PLAYBACK_FADEIN,  // args: fade-in
  MSG_SET_PLAYBACK_FADEOUT, // args: fade-out
  MSG_SET_PLAYBACK_CROSSFADE,
  MSG_MAIN_READY,           // the main process is ready to receive msgs
  MSG_MUSICQ_DATA_SUSPEND,  // args: queue number
  MSG_MUSICQ_DATA_RESUME,   // args: queue number

  /* from player */
  MSG_PLAY_PAUSEATEND_STATE,// args: 0/1
  MSG_PLAYBACK_BEGIN,       // to main: player has started playing the
                            //    song.  This message is sent after the
                            //    any announcement has finished.
  MSG_PLAYBACK_FINISH_STOP, // to main: player has finished playing song,
                            //    stop playing, do not continue
  MSG_PLAYBACK_FINISH,      // to main: player has finished playing song
                            //    args: song fname
  MSG_PLAYER_STATE,         // args: player state
  MSG_PLAYER_STATUS_DATA,   // response to get_status; to main
  MSG_PLAYER_ANN_FINISHED,  // announcement is finished

  /* to/from manageui/playerui */
  MSG_MUSIC_QUEUE_DATA,
  MSG_QUEUE_SWITCH,         // args: queue number
  MSG_SONG_SELECT,          // args: queue number, position
  MSG_FINISHED,             // no more songs, also sent to marquee
  MSG_SONG_FINISH,          // args: dbidx, for history
  MSG_MAIN_START_RECONN,    // the main process was already started
                            //  this message occurs after a crash or the ui
                            //  tries to start the main process again.
  MSG_MAIN_START_REATTACH,  // the main process was started before and
                            //  stayed active. re-attach to it.
  MSG_MAIN_REQ_STATUS,      // request current status from main
  MSG_MAIN_CURR_PLAY,       // status response: current play idx
  MSG_DB_ENTRY_TEMP_ADD,    // args: fn, dbidx, song-entry-text
  MSG_MAIN_REQ_QUEUE_INFO,  // args: queue number
                            // request complete queue information from main
  MSG_MAIN_QUEUE_INFO,      //
  MSG_PROCESSING_FINISH,    // essentially asks manageui to clear the status
  MSG_PROCESSING_FAIL,      //

  /* to/from starterui */
  MSG_START_MAIN,           // arg: true for --nomarquee
  MSG_STOP_MAIN,
  MSG_PROCESS_ACTIVE,     // arg: count of number of main start requests
  MSG_REQ_PROCESS_ACTIVE,
  MSG_DEBUG_LEVEL,          // arg: debug level

  /* to/from web servers */
  MSG_DANCE_LIST_DATA,      // args: html option list
  MSG_GET_DANCE_LIST,
  MSG_GET_PLAYLIST_LIST,    //
  MSG_MARQUEE_DATA,         // args: mq json data ; also for msg to marquee
  MSG_MUSICQ_STATUS_DATA,   // main response to remote control
  MSG_PLAYLIST_LIST_DATA,   // args: html option list
  MSG_CURR_SONG_DATA,

  /* to/from marquee */
  MSG_MARQUEE_TIMER,        // args: played time, duration
  MSG_MARQUEE_SET_FONT_SZ,  // args: font-size
  MSG_MARQUEE_FONT_SIZES,   // args: font-size, font-size-fs
  MSG_MARQUEE_HIDE,
  MSG_MARQUEE_SHOW,
  MSG_MARQUEE_STATUS,       // args: bool: isicon bool: ismax

  /* to/from dbupdate */
  MSG_DB_STOP_REQ,
  MSG_DB_PROGRESS,          // args: % complete
  MSG_DB_STATUS_MSG,        // args: status message
  MSG_DB_FINISH,            //
  MSG_DB_WAIT,              // display 'please wait'
  MSG_DB_WAIT_FINISH,       // clear status
  /* to/from bpm counter */
  MSG_BPM_TIMESIG,          // args: bpm/mpm time-signature(mpm)
  MSG_BPM_SET,              // args: bpm

  /* test-suite */
  MSG_CHK_MAIN_MUSICQ,
  MSG_CHK_PLAYER_STATUS,
  MSG_CHK_PLAYER_SONG,
  MSG_CHK_MAIN_RESET_SENT,
  MSG_CHK_WAIT_PREP,
  MSG_CHK_SET_DELAY,
  /* these commands act upon the queue currently set for playback */
  MSG_CHK_MAIN_SET_GAP,           // args: gap
  MSG_CHK_MAIN_SET_MAXPLAYTIME,   // args: max-play-time
  MSG_CHK_MAIN_SET_STOPATTIME,    // args: max-play-time
  MSG_CHK_MAIN_SET_CROSSFADE,
  MSG_CHK_MAIN_SET_PLAYANNOUNCE,  // args: true/false
  MSG_CHK_MAIN_SET_QUEUE_ACTIVE,  // args: true/false
  MSG_CHK_MAIN_SET_PLAY_WHEN_QUEUED,  // args: true/false
  MSG_CHK_MAIN_SET_STARTWAIT,     // args: start-wait
  MSG_CHK_MAIN_SET_FADEIN,        // args: fade-in
  MSG_CHK_MAIN_SET_FADEOUT,       // args: fade-out
  MSG_CHK_MAIN_SET_PAUSE_EACH_SONG, // args: true/false
  MSG_CHK_MAIN_SET_DANCESEL_METHOD,   // args: windowed/expectedcount

  MSG_CHK_CLEAR_PREP_Q,

  /* when a new message is added, update: */
  /* bdjmsg.c: debugging information for the msg */
  MSG_MAX,
} bdjmsgmsg_t;

enum {
  PREP_SONG,
  PREP_ANNOUNCE,
};

/* make the message size large enough to handle a */
/* 1000 (the max player queue length) long playlist message */
enum {
  BDJMSG_MAX_ARGS = 20000,
  /* the prefix consists of 3 4-char numerics with separators */
  /* and a null byte */
  BDJMSG_MAX_PFX = (sizeof (uint32_t) + 1) * 3 + 1,
  BDJMSG_MAX = BDJMSG_MAX_PFX + BDJMSG_MAX_ARGS,
};

extern char MSG_ARGS_RS;
extern const char *MSG_ARGS_RS_STR;
extern char MSG_ARGS_EMPTY;
extern const char *MSG_ARGS_EMPTY_STR;

/* exposed for use in testing */
extern const char *bdjmsgroutetxt [ROUTE_MAX];
extern const char *bdjmsgtxt [MSG_MAX];

size_t    msgEncode (bdjmsgroute_t routefrom, bdjmsgroute_t route, bdjmsgmsg_t msg, char *msgbuff, size_t mlen);
void      msgDecode (char *msgbuff, bdjmsgroute_t *routefrom, bdjmsgroute_t *route, bdjmsgmsg_t *msg, char **args);
const char *msgDebugText (bdjmsgmsg_t msg);
const char *msgRouteDebugText (bdjmsgroute_t route);

#if defined (__cplusplus) || defined (c_plusplus)
} /* extern C */
#endif

