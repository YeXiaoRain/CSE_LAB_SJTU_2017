#!/usr/bin/env bash

ulimit -c unlimited

if [ -z $NUM_LS ]; then
    NUM_LS=0
fi

#BASE_PORT=$RANDOM
BASE_PORT=777
BASE_PORT=$[BASE_PORT+2000]
EXTENT_PORT=$BASE_PORT
YFS1_PORT=$[BASE_PORT+2]
YFS2_PORT=$[BASE_PORT+4]

YFSDIR1=$PWD/yfs1
YFSDIR2=$PWD/yfs2

echo "starting ./extent_server $EXTENT_PORT > extent_server.log 2>&1 &"
./extent_server $EXTENT_PORT > extent_server.log 2>&1 &
sleep 1
