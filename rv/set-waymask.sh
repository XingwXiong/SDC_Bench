#!/bin/sh

if [ $# == 2 ]
then
    # separate redis-server, http-server, xapian and dwarfs into two groups
#    ps -ef | grep -v "grep" | grep -E "redis-server|http-server|xapian_networked_server" \
#        | awk '{system("echo "$1" > /sys/fs/cgroup/dsid/test-1/tasks")}'
#    ps -ef | grep -v "grep" | grep -E "single_thread|multi_thread" \
#        | awk '{system("echo "$1" > /sys/fs/cgroup/dsid/test-2/tasks")}'
    
    # set the two groups' dsid 
#    echo 1 > /sys/fs/cgroup/dsid/test-1/dsid.dsid-set
#    echo 2 > /sys/fs/cgroup/dsid/test-2/dsid.dsid-set
    
    #set the waymasks through arguments
    echo waymask $1 > /sys/fs/cgroup/dsid/test-1/dsid.dsid-cache
    echo waymask $2 > /sys/fs/cgroup/dsid/test-2/dsid.dsid-cache

else
    echo -e \
'''
error: unknown options.
usage:
    ./set-waymask.sh {waymask-1} {waymask-2}
'''
fi

