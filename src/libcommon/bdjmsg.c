/*
 * Copyright 2021-2025 Brad Lanam Pleasant Hill CA
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bdjmsg.h"
#include "bdjstring.h"

char MSG_ARGS_RS = 0x1E;     // RS
const char *MSG_ARGS_RS_STR = "\x1E";
char MSG_ARGS_EMPTY = 0x03;  // ETX
const char *MSG_ARGS_EMPTY_STR = "\x03";

enum {
  LSZ = sizeof (uint32_t),      /* four bytes */
};
static char MSG_RS = '~';

/* for debugging */
const char *bdjmsgroutetxt [ROUTE_MAX] = {
  [ROUTE_CONFIGUI] = "CONFIGUI",
  [ROUTE_DBUPDATE] = "DBUPDATE",
  [ROUTE_MAIN] = "MAIN",
  [ROUTE_MANAGEUI] = "MANAGEUI",
  [ROUTE_MARQUEE] = "MARQUEE",
  [ROUTE_MOBILEMQ] = "MOBILEMQ",
  [ROUTE_NONE] = "NONE",
  [ROUTE_PLAYER] = "PLAYER",
  [ROUTE_PLAYERUI] = "PLAYERUI",
  [ROUTE_REMCTRL] = "REMCTRL",
  [ROUTE_STARTERUI] = "STARTERUI",
  [ROUTE_HELPERUI] = "HELPERUI",
  [ROUTE_BPM_COUNTER] = "BPM_COUNTER",
  [ROUTE_TEST_SUITE] = "TEST_SUITE",
  [ROUTE_SERVER] = "SERVER",
  [ROUTE_ALTINST] = "ALTINST (NIU)",
};

/* for debugging */
const char *bdjmsgtxt [MSG_MAX] = {
  [MSG_BPM_SET] = "BPM_SET",
  [MSG_BPM_TIMESIG] = "BPM_TIMESIG",
  [MSG_CHK_CLEAR_PREP_Q] = "CHK_CLEAR_PREP_Q",
  [MSG_CHK_MAIN_MUSICQ] = "CHK_MAIN_MUSICQ",
  [MSG_CHK_MAIN_RESET_SENT] = "CHK_MAIN_RESET_SENT",
  [MSG_CHK_MAIN_SET_DANCESEL_METHOD] = "CHK_MAIN_SET_DANCESEL_METHOD",
  [MSG_CHK_MAIN_SET_STARTWAIT] = "CHK_MAIN_SET_STARTWAIT",
  [MSG_CHK_MAIN_SET_FADEIN] = "CHK_MAIN_SET_FADEIN",
  [MSG_CHK_MAIN_SET_FADEOUT] = "CHK_MAIN_SET_FADEOUT",
  [MSG_CHK_MAIN_SET_GAP] = "CHK_MAIN_SET_GAP",
  [MSG_CHK_MAIN_SET_MAXPLAYTIME] = "CHK_MAIN_SET_MAXPLAYTIME",
  [MSG_CHK_MAIN_SET_PAUSE_EACH_SONG] = "CHK_MAIN_SET_PAUSE_EACH_SONG",
  [MSG_CHK_MAIN_SET_PLAYANNOUNCE] = "CHK_MAIN_SET_PLAYANNOUNCE",
  [MSG_CHK_MAIN_SET_PLAY_WHEN_QUEUED] = "CHK_MAIN_SET_PLAY_WHEN_QUEUED",
  [MSG_CHK_MAIN_SET_QUEUE_ACTIVE] = "CHK_MAIN_SET_QUEUE_ACTIVE",
  [MSG_CHK_MAIN_SET_STOPATTIME] = "CHK_MAIN_SET_STOPATTIME",
  [MSG_CHK_PLAYER_SONG] = "CHK_PLAYER_SONG",
  [MSG_CHK_PLAYER_STATUS] = "CHK_PLAYER_STATUS",
  [MSG_CMD_NEXTSONG_PLAY] = "CMD_NEXTSONG_PLAY",
  [MSG_CMD_PLAY] = "CMD_PLAY",
  [MSG_CMD_PLAYPAUSE] = "CMD_PLAYPAUSE",
  [MSG_CURR_SONG_DATA] = "CURR_SONG_DATA",
  [MSG_DANCE_LIST_DATA] = "DANCE_LIST_DATA",
  [MSG_DB_RELOAD] = "DATABASE_UPDATE",
  [MSG_DB_LOADED] = "DB_UPDATED",
  [MSG_DB_ENTRY_TEMP_ADD] = "DB_ENTRY_TEMP_ADD",
  [MSG_DB_ENTRY_UPDATE] = "DB_ENTRY_UPDATE",
  [MSG_DB_ENTRY_REMOVE] = "DB_ENTRY_REMOVE",
  [MSG_DB_ENTRY_UNREMOVE] = "DB_ENTRY_UNREMOVE",
  [MSG_DB_FINISH] = "DB_FINISH",
  [MSG_DB_PROGRESS] = "DB_PROGRESS",
  [MSG_DB_STATUS_MSG] = "DB_STATUS_MSG",
  [MSG_DB_STOP_REQ] = "DB_STOP_REQ",
  [MSG_DB_WAIT] = "DB_WAIT",
  [MSG_DB_WAIT_FINISH] = "DB_WAIT_FINISH",
  [MSG_EXIT_REQUEST] = "EXIT_REQUEST",
  [MSG_FINISHED] = "FINISHED",
  [MSG_GET_DANCE_LIST] = "GET_DANCE_LIST",
  [MSG_GET_PLAYLIST_LIST] = "GET_PLAYLIST_LIST",
  [MSG_GET_STATUS] = "GET_STATUS",
  [MSG_HANDSHAKE] = "HANDSHAKE",
  [MSG_MAIN_START_RECONN] = "MAIN_START_RECONN",
  [MSG_MAIN_START_REATTACH] = "MAIN_START_REATTACH",
  [MSG_MAIN_CURR_PLAY] = "MAIN_CURR_PLAY",
  [MSG_MAIN_QUEUE_INFO] = "MAIN_QUEUE_INFO",
  [MSG_MAIN_READY] = "MAIN_READY",
  [MSG_MAIN_REQ_QUEUE_INFO] = "MAIN_REQ_QUEUE_INFO",
  [MSG_MAIN_REQ_STATUS] = "MAIN_REQ_STATUS",
  [MSG_MARQUEE_DATA] = "MARQUEE_DATA",
  [MSG_MARQUEE_FONT_SIZES] = "MARQUEE_FONT_SIZES",
  [MSG_MARQUEE_SET_FONT_SZ] = "MARQUEE_SET_FONT_SZ",
  [MSG_MARQUEE_TIMER] = "MARQUEE_TIMER",
  [MSG_MARQUEE_HIDE] = "MARQUEE_HIDE",
  [MSG_MARQUEE_SHOW] = "MARQUEE_SHOW",
  [MSG_MARQUEE_STATUS] = "MARQUEE_STATUS",
  [MSG_MUSICQ_DATA_RESUME] = "MUSICQ_DATA_RESUME",
  [MSG_MUSICQ_DATA_SUSPEND] = "MUSICQ_DATA_SUSPEND",
  [MSG_MUSICQ_INSERT] = "MUSICQ_INSERT",
  [MSG_MUSICQ_MOVE_DOWN] = "MUSICQ_MOVE_DOWN",
  [MSG_MUSICQ_MOVE_TOP] = "MUSICQ_MOVE_TOP",
  [MSG_MUSICQ_MOVE_UP] = "MUSICQ_MOVE_UP",
  [MSG_MUSICQ_REMOVE] = "MUSICQ_REMOVE",
  [MSG_MUSICQ_SWAP] = "MUSICQ_SWAP",
  [MSG_MUSICQ_SET_LEN] = "MUSICQ_SET_LEN",
  [MSG_MUSICQ_SET_PLAYBACK] = "MUSICQ_SET_PLAYBACK",
  [MSG_MUSICQ_STATUS_DATA] = "MUSICQ_STATUS_DATA",
  [MSG_MUSICQ_TOGGLE_PAUSE] = "MUSICQ_TOGGLE_PAUSE",
  [MSG_MUSICQ_TRUNCATE] = "MUSICQ_TRUNCATE",
  [MSG_MUSIC_QUEUE_DATA] = "MUSIC_QUEUE_DATA",
  [MSG_NULL] = "NULL",
  [MSG_PLAYBACK_BEGIN] = "PLAYBACK_BEGIN",
  [MSG_PLAYBACK_FINISH] = "PLAYBACK_FINISH",
  [MSG_PLAYBACK_FINISH_STOP] = "PLAYBACK_FINISH_STOP",
  [MSG_PLAYER_STATE] = "PLAYER_STATE",
  [MSG_PLAYER_STATUS_DATA] = "PLAYER_STATUS_DATA",
  [MSG_PROCESS_ACTIVE] = "PROCESS_ACTIVE",
  [MSG_PLAYER_ANN_FINISHED] = "PLAYER_ANN_FINISHED",
  [MSG_PLAYER_VOL_MUTE] = "PLAYER_VOL_MUTE",
  [MSG_PLAYER_VOLUME] = "PLAYER_VOLUME",
  [MSG_PLAYER_SUPPORT] = "PLAYER_SUPPORT",
  [MSG_PLAY_FADE] = "PLAY_FADE",
  [MSG_PLAYLIST_LIST_DATA] = "PLAYLIST_LIST_DATA",
  [MSG_PLAY_NEXTSONG] = "PLAY_NEXTSONG",
  [MSG_PLAY_PAUSEATEND] = "PLAY_PAUSEATEND",
  [MSG_PLAY_PAUSEATEND_STATE] = "PLAY_PAUSEATEND_STATE",
  [MSG_PLAY_PAUSE] = "PLAY_PAUSE",
  [MSG_PLAY_PLAYPAUSE] = "PLAY_PLAYPAUSE",
  [MSG_PLAY_PLAY] = "PLAY_PLAY",
  [MSG_PLAY_REPEAT] = "PLAY_REPEAT",
  [MSG_PLAY_SEEK] = "PLAY_SEEK",
  [MSG_PLAY_SONG_BEGIN] = "PLAY_SONG_BEGIN",
  [MSG_PLAY_SPEED] = "PLAY_SPEED",
  [MSG_PLAY_STOP] = "PLAY_STOP",
  [MSG_PLAY_RESET_VOLUME] = "PLAY_RESET_VOLUME",
  [MSG_PL_CLEAR_QUEUE] = "PL_CLEAR_QUEUE",
  [MSG_PL_OVERRIDE_STOP_TIME] = "PL_OVERRIDE_STOP_TIME",
  [MSG_PROCESSING_FAIL] = "PROCESSING_FAIL",
  [MSG_PROCESSING_FINISH] = "PROCESSING_FINISH",
  [MSG_QUEUE_CLEAR] = "QUEUE_CLEAR",
  [MSG_QUEUE_DANCE] = "QUEUE_DANCE",
  [MSG_QUEUE_MIX] = "QUEUE_MIX",
  [MSG_QUEUE_PLAYLIST] = "QUEUE_PLAYLIST",
  [MSG_QUEUE_SWITCH_EMPTY] = "QUEUE_SWITCH_EMPTY",
  [MSG_QUEUE_SWITCH] = "QUEUE_SWITCH",
  [MSG_REQ_PROCESS_ACTIVE] = "REQ_PROCESS_ACTIVE",
  [MSG_DEBUG_LEVEL] = "DEBUG_LEVEL",
  [MSG_SET_PLAYBACK_FADEIN] = "SET_PLAYBACK_FADEIN",
  [MSG_SET_PLAYBACK_FADEOUT] = "SET_PLAYBACK_FADEOUT",
  [MSG_SET_PLAYBACK_GAP] = "SET_PLAYBACK_GAP",
  [MSG_SOCKET_CLOSE] = "SOCKET_CLOSE",
  [MSG_SONG_CLEAR_PREP] = "SONG_CLEAR_PREP",
  [MSG_SONG_FINISH] = "SONG_FINISH",
  [MSG_SONG_PLAY] = "SONG_PLAY",
  [MSG_SONG_PREP] = "SONG_PREP",
  [MSG_SONG_SELECT] = "SONG_SELECT",
  [MSG_START_MAIN] = "START_MAIN",
  [MSG_START_MARQUEE] = "START_MARQUEE",
  [MSG_STOP_MARQUEE] = "STOP_MARQUEE",
  [MSG_STOP_MAIN] = "STOP_MAIN",
  [MSG_WINDOW_FIND] = "WINDOW_FIND",
};

size_t
msgEncode (bdjmsgroute_t routefrom, bdjmsgroute_t route,
    bdjmsgmsg_t msg, char *msgbuff, size_t mlen)
{
  size_t      len;

  len = (size_t) snprintf (msgbuff, mlen, "%0*d%c%0*d%c%0*d%c",
      LSZ, routefrom, MSG_RS, LSZ, route, MSG_RS, LSZ, msg, MSG_RS);
  msgbuff [mlen - 1] = '\0';
  if (len >= mlen) {
    len = mlen - 1;
  }
  /* always send the null byte so that there will be an empty args string */
  /* if the additional args are not specified */
  ++len;
  return len;
}

void
msgDecode (char *msgbuff, bdjmsgroute_t *routefrom, bdjmsgroute_t *route,
    bdjmsgmsg_t *msg, char **args)
{
  char        *p = NULL;

  p = msgbuff;
  *routefrom = (bdjmsgroute_t) atol (p);
  p += LSZ + 1;
  *route = (bdjmsgroute_t) atol (p);
  p += LSZ + 1;
  *msg = (bdjmsgmsg_t) atol (p);
  p += LSZ + 1;
  if (args != NULL) {
    *args = p;
  }
}

const char *
msgDebugText (bdjmsgmsg_t msg)
{
  return bdjmsgtxt [msg];
}

const char *
msgRouteDebugText (bdjmsgroute_t route)
{
  return bdjmsgroutetxt [route];
}

