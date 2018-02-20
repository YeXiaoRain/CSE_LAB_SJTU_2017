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
LOCK_PORT=$[BASE_PORT+6]
#Please set this two server IPs 
EXTENT_SERVER_HOST=127.0.0.1
LOCK_SERVER_HOST=127.0.0.1

YFSDIR1=$PWD/yfs1
YFSDIR2=$PWD/yfs2

rm -rf $YFSDIR1
mkdir $YFSDIR1 || exit 1
sleep 1
echo "starting ./yfs_client $YFSDIR1 $EXTENT_SERVER_HOST:$EXTENT_PORT $LOCK_SERVER_HOST:$LOCK_PORT > yfs_client1.log 2>&1 &"
./yfs_client $YFSDIR1 $EXTENT_SERVER_HOST:$EXTENT_PORT $LOCK_SERVER_HOST:$LOCK_PORT > yfs_client1.log 2>&1 &
sleep 1

rm -rf $YFSDIR2
mkdir $YFSDIR2 || exit 1
sleep 1
echo "starting ./yfs_client $YFSDIR2 $EXTENT_SERVER_HOST:$EXTENT_PORT $LOCK_SERVER_HOST:$LOCK_PORT > yfs_client2.log 2>&1 &"
./yfs_client $YFSDIR2 $EXTENT_SERVER_HOST:$EXTENT_PORT $LOCK_SERVER_HOST:$LOCK_PORT > yfs_client2.log 2>&1 &

sleep 2

# make sure FUSE is mounted where we expect
pwd=`pwd -P`
if [ `mount | grep "$pwd/yfs1" | grep -v grep | wc -l` -ne 1 ]; then
    sh stop.sh
    echo "Failed to mount YFS properly at ./yfs1"
    exit -1
fi

# make sure FUSE is mounted where we expect
if [ `mount | grep "$pwd/yfs2" | grep -v grep | wc -l` -ne 1 ]; then
    sh stop.sh
    echo "Failed to mount YFS properly at ./yfs2"
    exit -1
fi
