#!/bin/sh

prefix=/home/xingw/mb-redis-4.0.2/deps/jemalloc/install
exec_prefix=/home/xingw/mb-redis-4.0.2/deps/jemalloc/install
libdir=${exec_prefix}/lib

LD_PRELOAD=${libdir}/libjemalloc.so.2
export LD_PRELOAD
exec "$@"
