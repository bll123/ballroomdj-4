#!/bin/bash

valgrind \
    --num-callers=40 \
    --trace-children=yes \
    --track-origins=yes \
    --leak-check=full \
    --log-file="ww.v" \
    ../bin/bdj4

#    --show-leak-kinds=all
