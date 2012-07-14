#!/bin/sh
echo "USAGE: \$ run_rc.sh MAX_PARTITION_SIZE CACHE_SIZE MIN_PARTITION_SIZE"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:commons/Debug:memcached-1.6/.libs/:"
export MALLOC_TRACE="rc/Mtrace.dat"
./memcached-1.6/.libs/memcached -vv -p 11212 -E rc/Debug/librc.so -I $1 -m $2 -n $3
