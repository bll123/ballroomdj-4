# 2025-10-10
#
#   145-10 must use specific songs...
#     the musicq-insert uses specific indices
#     this is icky.
#     see SONGIDX.txt
#
# The testsuite program uses set-delay of 450 to delay the prep-time
# in order to simulate slow loading of songs.
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
#     display responses *after* using 'chk' or 'wait'
#     does not work before or without 'chk' or 'wait'
#   reset
#     resets the player
#   end
#     resets the player, ends the test, prints results
#   file {rm|exists|not-exists|copy} <fn> [<fn2>]
#   get <route> <msg>
#   msg <route> <msg>
#   mssleep <time>
#   crossfadechk
#     if not supported, skip the test
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
#   CHK_MAIN_MUSICQ
#     mqplay mq0len mq1len m-songfn title dance
#     dbidx
#       only if playing, from the musicq-play-idx
#     qdbidx
#       only if not playing, from the musicq-play-idx
#     mqXidxN (mq 0-1 idx 0-5)
#     paeX (pause-at-end 0-5)
# player:
#   CHK_PLAYER_STATUS
#     playstate
#       unknown stopped loading playing paused in-fadeout in-gap in-crossfade
#     plistate
#       none opening buffering playing paused stopped ended error
#         note that these may be specific to the particular player interface.
#         the only states that should be used in tests
#         are 'playing' and 'paused'.
#     currvolume realvolume actualvolume
#       actualvolume is used to test fade-in and fade-out
#     plivolume
#     speed playtimeplayed pauseatend repeat currentsink
#     prepqueuecount
#       check the prepqueuecount can be problematical as it can take
#       differing amounts of time to do the prep.
#       using a artificial delay in the prep code is useful to verify
#       the test
#     incrossfade
# song:
#   CHK_PLAYER_SONG
#     p-duration p-songfn
#   CHK_WAIT_PREP
#     done
#
