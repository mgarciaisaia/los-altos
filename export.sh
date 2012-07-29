if [[ $_ == $0 ]] ; then
    echo "No estas ejecutando con .";
    echo "Ejecutalo asi:";
    echo ". "$0 $*
    exit 1
fi
repoDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export LD_LIBRARY_PATH=$repoDir/commons/Debug:$repoDir/rc/Debug:$repoDir/memcached-1.6/.libs
export MALLOC_TRACE=$repoDir/rc/Mtrace.dat
alias rfs="cd $repoDir/rfs/Debug && ./rfs"
alias fsc="cd $repoDir/fsc/Debug && ./fsc"
alias rc="cd $repoDir/memcached-1.6/.libs && ./memcached -vv -p 11212 -E $repoDir/rc/Debug/librc.so"
