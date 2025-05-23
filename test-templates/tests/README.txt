# 2025-1-4
#
# Note that the musicq chk commands currently return the internal musicq
# index, which is offset by one.
#
# test songs are all between 29 and 32 seconds long
#   excepting four jive songs for testing song-start, song-end and
#   no-max-play-time
# fade-out default: 4000
# gap default: 2000
#
# 32+ seconds - 4 = 28, timeout 29000 to in-fadeout
# 32+ seconds + 2 = 34, timeout 35000 to next song
# playing the entire song.
#
# 'defaultvol' is replaced with the default volume
# 'stoptime' is replaced with current time + two minutes
# '%HISTQ%' is replaced with the history queue number
#
# commands:
#   chk {key value} ...
#     special values: defaultvol
#     (chk-not, chk-or, chk-lt, chk-gt)
#   disp, dispall
#     display responses before using 'chk' or 'wait'
#   reset
#     resets the player
#   end
#     resets the player, ends the test, prints results
#   file {rm|exists|not-exists|copy} <fn> [<fn2>]
#   get <route> <msg>
#   msg <route> <msg>
#   mssleep <time>
#   print <text>
#   resptimeout <time>
#     set the response timeout for 'wait'
#   section <num> <name>
#   skip
#     skips the rest of the test file.
#   stop
#     for debugging; stops processing
#   test <num> <name>
#   verbose {on|off}
#   wait {key value}
#     (wait-not)
#
# main:
#   musicq: CHK_MAIN_MUSICQ
#     mqplay mq0len mq1len m-songfn title dance
#     dbidx
#       only if playing, from the musicq-play-idx
#     qdbidx
#       only if not playing, from the musicq-play-idx
#     mqXidxN (mq 0-1 idx 0-5)
#     paeX (pause-at-end 0-5)
# player:
#   status: CHK_PLAYER_STATUS
#     playstate
#       unknown stopped loading playing paused in-fadeout in-gap
#     plistate
#       none opening buffering playing paused stopped ended error
#         note that these may be specific to the particular player interface.
#         the only states that should be used in tests
#         are playing and paused.
#     currvolume realvolume actualvolume
#       actualvolume is used to test fade-in and fade-out
#     plivolume
#     speed playtimeplayed
#     pauseatend repeat
#     currentsink
#     prepqueuecount
#   song: CHK_PLAYER_SONG
#     p-duration p-songfn
#
