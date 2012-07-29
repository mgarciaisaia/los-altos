if [[ $_ == $0 ]] ; then
    echo "No estas ejecutando con .";
    echo "Ejecutalo asi:";
    echo ". "$0 $*
    exit 1
fi
repoDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd
mkdir Desarrollo
truncate --size 0 Desarrollo/ext2.disk
echo "Voy a ejecutar: sudo mkfs.ext2 -F -O none,sparse_super -b 1024 -I 128 Desarrollo/ext2.disk 2000000"
sudo mkfs.ext2 -F -O none,sparse_super -b 1024 -I 128 Desarrollo/ext2.disk 2000000
cd
mkdir -p tmp/fuse
cd $repoDir
./compile.sh
. ./export.sh
