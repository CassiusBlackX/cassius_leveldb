#!/bin/bash

EXECUTABLE="../build/ex_kv_io_raid"


VALUES=(1 2 4 16 32 64)

for VALUE in "${VALUES[@]}"
do
    for INDEX in {1..5}
    do
        DATE=$(date +%m%d)
        LOG_FILE="../log/raid_kv${VALUE}K_16G_${INDEX}_${DATE}.log"
        $EXECUTABLE $VALUE > $LOG_FILE 2>&1
    done
done