#!/bin/bash
#TAIL_DIR=/mnt/tailbench
TAIL_DIR=${PWD}/../
source ${TAIL_DIR}/configs.sh
NSERVERS=1
WARMUPREQS=900
REQUESTS=3000
#WARMUPREQS=100
#REQUESTS=500

TBENCH_SERVER_PORT=10000 \
TBENCH_RANDSEED=${RANDOM} TBENCH_MAXREQS=${REQUESTS} TBENCH_WARMUPREQS=${WARMUPREQS} \
    taskset 0x1 chrt -r 99 ${TAIL_DIR}/xapian/xapian_networked_server -n ${NSERVERS} -d ${DATA_ROOT}/xapian/books \
    -r 1000000000 &

while [ 1 ]
do
    netstat -nlp | grep 10000
    if [ $? == 0 ]
      then
          echo start ok
          break
      else
          echo -e '#\c'
    fi
done

ps -ef | grep -v "grep" | grep "xapian_networked_server" | awk 'system("echo "$1"  > /sys/fs/cgroup/dsid/test-1/tasks")'
