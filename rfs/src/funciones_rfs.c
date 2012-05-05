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
#include "funciones_rfs.h"

#define PATH "/home/utnso/Desarrollo/ext2.disk"
#define BLOQUE 1024

uint32_t *ptr_arch;

void mapear_archivo() {

	uint32_t archivo = open(PATH, O_RDWR);
	if (archivo == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	struct stat fd;
	fstat(archivo, &fd);
//	printf("El archivo pesa: %ld bytes \n",(long int)fd.st_size);

	ptr_arch = mmap(NULL, fd.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,archivo,0);
	if (ptr_arch == MAP_FAILED) {
		close(archivo);
		perror("Error mapping the file");
		exit(EXIT_FAILURE);
	}

	if (posix_madvise(ptr_arch, fd.st_size, POSIX_MADV_NORMAL) < 0)
		perror("posix_madvise");

}


/*aca seria la funcion de leer el superbloque *****************/

struct Superblock *read_superblock(){

	uint32_t OFFSET = 256;
	struct Superblock *bloque;
	uint32_t *posicion = ptr_arch +OFFSET;
	bloque= (struct Superblock *) posicion;

	return bloque;

}

uint32_t cantidadDeGrupos(uint32_t inodos,uint32_t inodosPorGrupo){
	double resto = inodos % inodosPorGrupo;
	int cantidadDeGrupos = inodos/inodosPorGrupo;
	return (resto != 0) ? cantidadDeGrupos + 1 : cantidadDeGrupos;
}

/**/

void leerLosGroupDescriptor(uint32_t inodosTotales,uint32_t inodosPorGrupo){

//	struct Superblock *bloque = read_superblock(datos);

//	inodosTotales = bloque->inodes;
//	inodosPorGrupo = bloque->inodes_per_group;
	uint32_t cantDeGrupos = cantidadDeGrupos(inodosTotales,inodosPorGrupo);

	struct GroupDesc * grupo;

//	int posInicialGD = 512;
	int32_t tamanioGrupo = sizeof(struct GroupDesc);
	printf("Bytes de un grupo: %d\n",tamanioGrupo);
	uint32_t *posicion2;

	uint32_t i;
	int posInicialGD = 512;
		posicion2 = ptr_arch+posInicialGD;
		for(i=0;i < cantDeGrupos;i++){

		grupo = (struct GroupDesc*) posicion2;

		printf("Group Descriptor N%d\n",i);
		printf("block_bitmap: %d\n",grupo->block_bitmap);
		printf("inode_bitmap: %d\n",grupo->inode_bitmap);
		printf("inode_table: %d\n",grupo->inode_table);
		printf("free_blocks_count: %d\n",grupo->free_blocks_count);
		printf("free_inodes_count: %d\n",grupo->free_inodes_count);
		printf("used_dirs_count: %d\n\n",grupo->used_dirs_count);

		// posiciona el puntero a la siguiente estructura de grupo
		posicion2 += tamanioGrupo;
	//					int i;
	//					for(i = 0; i < 7; i++)
	//						printf("dummy[%d]: %d\n",i,grupo->dummy[i]);
		}

// return grupo;

}

char esPotenciaDe(uint32_t grupo){

	char resultado = 0;
	return resultado;

}

uint32_t *dir_inicioDeGrupo (uint32_t grupo){

	uint32_t *dir_inicio;

	struct Superblock *bloque = read_superblock();

	uint32_t bloqueMasTablaDG = BLOQUE + (cantidadDeGrupos(bloque->inodes,bloque->inodes_per_group) * (sizeof(struct GroupDesc)));
	uint32_t *posicion = ptr_arch + bloqueMasTablaDG;


	if (grupo == 0)
		dir_inicio = posicion + 256;

		else {
			if (grupo == esPotenciaDe(grupo))
				dir_inicio = posicion + (grupo * bloque->blocks_per_group);

				else
					dir_inicio = (uint32_t *)(grupo * bloque->blocks_per_group);
		}

	return dir_inicio;
}

