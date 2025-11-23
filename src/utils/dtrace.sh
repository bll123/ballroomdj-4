#!/bin/bash

script='pid$target:a.out::entry,pid$target:a.out::return { trace(arg1); }'
sudo dtrace -f -n "${script}" -o out.txt -c "./bin/bdj4"
#sudo dtrace -f -n "${script}" -o out.txt -c "./bin/bdj4starterui --bdj4 --origcwd $(pwd)"
