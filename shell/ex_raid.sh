#!/bin/bash

EXECUTABLE="../build/ex_kv_io"


VALUES=(1 2 4 16 32 64)

for VALUE in "${VALUES[@]}"
do
    for INDEX in {1..5}
    do
        DATE=$(date +%m%d)
        LOG_FILE="../log/raid_kv${VALUE}K_2G_${INDEX}_${DATE}.log"
        WORKLOAD_FILE_NAME="workload_${VALUE}K.ini"
        PROPERTY_FILE_NAME="raid.properties"
        $EXECUTABLE $WORKLOAD_FILE_NAME $PROPERTY_FILE_NAME > $LOG_FILE 2>&1
    done
done