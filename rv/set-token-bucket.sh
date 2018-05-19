#!/bin/sh
DSID_DIR=/sys/fs/cgroup/dsid

if [ $# == 4 ]
then
    echo sizes $2 > ${DSID_DIR}/test-$1/dsid.dsid-mem
    echo inc   $3 > ${DSID_DIR}/test-$1/dsid.disd-mem
    echo freq  $4 > ${DSID_DIR}/test-$1/dsid.disd-mem
else
    echo -e \
'''
error:
    unknown options
usage:
    ./set-token-bucket.sh {test-id} {sizes} {inc} {freq}
'''
fi
