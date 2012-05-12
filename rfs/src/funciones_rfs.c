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
#include <math.h>

#define PATH "/home/utnso/Desarrollo/ext2.disk"
#define BLOQUE 1024

//uint32_t *ptr_arch;
uint8_t *ptr_arch;

void mapear_archivo() {

	uint32_t archivo = open(PATH, O_RDWR);
	if (archivo == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	struct stat fd;
	fstat(archivo, &fd);
//	printf("El archivo pesa: %ld bytes \n",(long int)fd.st_size);

	ptr_arch = mmap(NULL, fd.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,archivo
			,0);
	if (ptr_arch == MAP_FAILED) {
		close(archivo);
		perror("Error mapping the file");
		exit(EXIT_FAILURE);
	}

	if (posix_madvise(ptr_arch, fd.st_size, POSIX_MADV_NORMAL) < 0)
		perror("posix_madvise");

}

/*aca seria la funcion de leer el superbloque *****************/

struct Superblock *read_superblock() {

	uint16_t OFFSET = 1024;
	struct Superblock *bloque;
	uint8_t *posicion = ptr_arch + OFFSET;
	bloque = (struct Superblock *) posicion;

	return bloque;

}

uint32_t cantidadDeGrupos(uint32_t inodos,uint32_t inodosPorGrupo) {
	double resto = inodos % inodosPorGrupo;
	int cantidadDeGrupos = inodos / inodosPorGrupo;
	return (resto != 0) ? cantidadDeGrupos + 1 : cantidadDeGrupos;
}

void leerLosGroupDescriptor(uint32_t inodosTotales,uint32_t inodosPorGrupo) {

	uint32_t cantDeGrupos = cantidadDeGrupos(inodosTotales,inodosPorGrupo);

	struct GroupDesc * grupo;

	uint16_t posInicialGD = 2048;
	int32_t tamanioGrupo = sizeof(struct GroupDesc);
	printf("Bytes de un grupo: %d\n", tamanioGrupo);
	uint8_t *posActual;

	uint32_t i;
		posActual = ptr_arch+posInicialGD;
		for(i=0;i < cantDeGrupos;i++) {

		grupo = (struct GroupDesc*) posActual;

		printf("Group Descriptor Nro %d\n",i);
		printf("block_bitmap: %d\n",grupo->block_bitmap);
		printf("inode_bitmap: %d\n",grupo->inode_bitmap);
		printf("inode_table: %d\n",grupo->inode_table);
		printf("free_blocks_count: %d\n",grupo->free_blocks_count);
		printf("free_inodes_count: %d\n",grupo->free_inodes_count);
		printf("used_dirs_count: %d\n\n",grupo->used_dirs_count);

		posActual += tamanioGrupo;

		}
}

struct GroupDesc * leerGroupDescriptor(uint32_t nroGrupo){
	struct GroupDesc *grupo;

	uint16_t posInicialGD = 2048;
	uint8_t tamanioGrupo = sizeof(struct GroupDesc);
	uint8_t *posActual = ptr_arch + posInicialGD + (tamanioGrupo * nroGrupo);

	grupo = (struct GroupDesc*) posActual;

	return grupo;
}

uint8_t *dir_inicioDeGrupo(uint32_t grupo) {

	struct GroupDesc *TDgrupo = leerGroupDescriptor(grupo);
	uint8_t *dir_bitmap_bloque = ptr_arch + (TDgrupo->block_bitmap * 1024);
	printf("direccion del grupo 1= %p, \n",dir_bitmap_bloque);
	return dir_bitmap_bloque;

}

void leerBitmapDeBloque(uint32_t nroGrupo){

	struct CampoBits * cb;
	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
	uint8_t bloques_libres = grupo->free_blocks_count;
	printf("Bloques libre: %hu\n",bloques_libres);

	uint8_t nroBloque = grupo->block_bitmap;
	uint8_t *posActual = ptr_arch + nroBloque * 1024;

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->blocks_per_group;
	printf("Bloques por grupo: %d\n",cantBloques);
	int cantIter = cantBloques / 8;

	uint8_t tamanioUnByte = 1;
	int i;
	for(i = 0;i < cantIter;i++){
		cb = (struct CampoBits*)posActual;
		printf("b: %u\n",cb->b0);
		printf("b1: %u\n",cb->b1);
		printf("b2: %u\n",cb->b2);
		printf("b3: %u\n",cb->b3);
		printf("b4: %u\n",cb->b4);
		printf("b5: %u\n",cb->b5);
		printf("b6: %u\n",cb->b6);
		printf("b7: %u\n",cb->b7);
		posActual += tamanioUnByte;
	}

}

void leerBitmapDeInodos(uint32_t nroGrupo){

	struct CampoBits * cb;

	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
	uint8_t inodos_libres = grupo->free_inodes_count;
	printf("inodos_libres: %hu\n",inodos_libres);

	uint8_t nroBloque = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + nroBloque * 1024;

	struct Superblock *bloque = read_superblock();
	int cantInodos = bloque->inodes_per_group;
	printf("cantidad de inodos por grupo: %d\n",cantInodos);
	int cantIter = cantInodos / 8;
	int i;
	uint8_t tamanioUnByte = 1;
	for(i = 0;i < cantIter;i++){
		cb = (struct CampoBits*)posActual;
		printf("b: %u\n",cb->b0);
		printf("b1: %u\n",cb->b1);
		printf("b2: %u\n",cb->b2);
		printf("b3: %u\n",cb->b3);
		printf("b4: %u\n",cb->b4);
		printf("b5: %u\n",cb->b5);
		printf("b6: %u\n",cb->b6);
		printf("b7: %u\n",cb->b7);
		posActual += tamanioUnByte;
	}

}

void leerTablaDeInodos(uint32_t nroGrupo){
	struct Superblock *bloque = read_superblock();
	uint32_t cantInodos = bloque->inodes_per_group;
	printf("cantidad de inodos %d\n",cantInodos);

	uint8_t tamanioInodo = sizeof(struct INode);
	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
	uint8_t nroBloque = grupo->inode_table;
	uint8_t *dir_tabla_inodos = ptr_arch + nroBloque * 1024;
	struct INode* inodo;
	int i;
	for(i=1;i<=cantInodos;i++){
		inodo = (struct INode*) dir_tabla_inodos;

		printf("inodo nro %d\n",i);
		printf("mode: %hu\n",inodo->mode);
		printf("uid: %hu\n",inodo->uid);
		printf("atime: %u\n",inodo->atime);

		dir_tabla_inodos += tamanioInodo;
	}
//	printf("inodos libres de este grupo: %hu",grupo->free_inodes_count);
}
