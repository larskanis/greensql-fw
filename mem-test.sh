#!/bin/sh

valgrind --track-fds=yes --log-file=./green-mem-log --num-callers=50 --leak-check=full --leak-resolution=high --show-reachable=yes ./greensql-fw -p ./conf/
