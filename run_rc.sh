#!/bin/sh
export LD_LIBRARY_PATH="$(pwd)/commons/Debug:$(pwd)/memcached-1.6/.libs/"
export MALLOC_TRACE="rc/Mtrace.dat"
if [ $# -eq 2 ]
then
	./memcached-1.6/.libs/memcached -vv -p 11212 -E rc/Debug/librc.so -m $1 -n $2
elif [ $# -eq 3 ]
then
	./memcached-1.6/.libs/memcached -vv -p 11212 -E rc/Debug/librc.so -I $1 -m $2 -n $3
else
	echo "USAGE:\n\$ run_rc.sh [ MAX_PARTITION_SIZE ] CACHE_SIZE MIN_PARTITION_SIZE"
fi
