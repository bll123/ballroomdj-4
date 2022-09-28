# 2022-9-28
#
# test songs are all between 29 and 32 seconds long
# fade-out default: 4000
# gap default: 2000
#
# 32+ seconds + 2 = 34, so a timeout of 35 seconds is used if
# playing the entire song.
#
# commands:
#   section test msg get chk wait mssleep
#   resptimeout
#     set the response timeout for 'wait'
#   reset end
#     ends the test
#   disp
#     display responses before using 'chk' or 'wait'
#
# main:
#   status:
#     mqmanage mqplay mq0len mq1len m-songfn title dance
#     dbidx
#       only if playing, from the musicq-play-idx
#     qdbidx
#       only if not playing, from the musicq-play-idx
#     mqXidxN (mq 0-1 idx 0-5)
# player:
#   status:
#     playstate
#       unknown stopped loading playing paused in-fadeout in-gap
#     plistate
#       none opening buffering playing paused stopped ended error
#         note that these may be specific to vlc.
#         the only states that should be used in tests
#         are playing and paused.
#     currvolume realvolume speed playtimeplayed
#     pauseatend repeat defaultsink currentsink
#   song:
#     p-duration p-songfn
#
# MUSICQ_CURRENT == 6
#
