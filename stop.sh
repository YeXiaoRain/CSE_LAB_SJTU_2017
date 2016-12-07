#!/usr/bin/env bash

YFSDIR0=$PWD/yfs0
YFSDIR1=$PWD/yfs1
YFSDIR2=$PWD/yfs2
YFSDIR3=$PWD/yfs3

export PATH=$PATH:/usr/local/bin
UMOUNT="umount"
if [ -f "/usr/local/bin/fusermount" -o -f "/usr/bin/fusermount" -o -f "/bin/fusermount" ]; then
    UMOUNT="fusermount -u";
fi
$UMOUNT $YFSDIR0
$UMOUNT $YFSDIR1
$UMOUNT $YFSDIR2
$UMOUNT $YFSDIR3
killall extent_server
killall yfs_client
killall lock_server
