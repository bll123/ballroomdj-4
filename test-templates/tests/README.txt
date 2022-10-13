# 2022-9-28
#
# test songs are all between 29 and 32 seconds long
# fade-out default: 4000
# gap default: 2000
#
# 32+ seconds - 4 = 28, timeout 29000 to in-fadeout
# 32+ seconds + 2 = 34, timeout 35000 to next song
# playing the entire song.
#
# commands:
#   section <num> <name>
#   test <num> <name>
#   msg <route> <msg>
#   get <route> <msg>
#   chk {key value} ...
#     special values: defaultvol
#   wait {key value}
#   mssleep <time>
#   resptimeout <time>
#     set the response timeout for 'wait'
#   reset, end
#     ends the test
#   disp, dispall
#     display responses before using 'chk' or 'wait'
#   stop
#     for debugging; stops processing
#
# main:
#   musicq:
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
#         note that these may be specific to the particular player interface.
#         the only states that should be used in tests
#         are playing and paused.
#     currvolume realvolume actualvolume
#       actualvolume is used to test fade-in and fade-out
#     speed playtimeplayed
#     pauseatend repeat
#     defaultsink currentsink
#     prepqueuecount
#   song:
#     p-duration p-songfn
#
# MUSICQ_CURRENT == 6
#
