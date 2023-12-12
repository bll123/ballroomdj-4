#!/bin/bash

valgrind \
    --num-callers=40 \
    --trace-children=yes \
    --track-origins=yes \
    --leak-check=full \
    ../bin/bdj4

#    --show-leak-kinds=all
