/*
 * funciones_rfs.c
 *
 *  Created on: 03/05/2012
 *      Author: utnso
 */

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#define PATH "/home/utnso/Desarrollo/ext2.disk"

uint32_t *mapear_archivo() {

	uint32_t archivo = open(PATH, O_RDWR);
	if (archivo == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	struct stat fd;
	fstat(archivo, &fd);
//	printf("El archivo pesa: %ld bytes \n",(long int)fd.st_size);

	uint32_t *p = mmap(NULL, fd.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			archivo, 0);
	if (p == MAP_FAILED) {
		close(archivo);
		perror("Error mapping the file");
		exit(EXIT_FAILURE);
	}

	if (posix_madvise(p, fd.st_size, POSIX_MADV_NORMAL) < 0)
		perror("posix_madvise");


	return p;

}


/*aca seria la funcion de leer el superbloque *****************/

struct Superblock *read_superblock(uint32_t *datos){

	uint32_t OFFSET = 256;
	struct Superblock *bloque;
	uint32_t *posicion = datos+OFFSET;
	bloque= (struct Superblock *) posicion;

	return bloque;

}




