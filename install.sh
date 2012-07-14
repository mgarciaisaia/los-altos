#!/bin/sh
cd
mkdir Desarrollo
touch Desarrollo/ext2.disk
echo "Voy a ejecutar: sudo mkfs.ext2 -F -O none,sparse_super -b 1024 -I 128 Desarrollo/ext2.disk 2000000"
sudo mkfs.ext2 -F -O none,sparse_super -b 1024 -I 128 Desarrollo/ext2.disk 2000000
cd
mkdir -p tmp/fuse
