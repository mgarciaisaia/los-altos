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
#include <src/commons/bitarray.h>
//#include <commons/bitarray.h>
#include <src/commons/collections/queue.h>
//#include <commons/collections/queue.h>

#define PATH "/home/utnso/Desarrollo/ext2.disk"
#define BLOQUE 1024

static const int tamanio_bloque = 1024;
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
	uint8_t *dir_bitmap_bloque = ptr_arch + (TDgrupo->block_bitmap * tamanio_bloque);
	printf("direccion del grupo 1= %p, \n",dir_bitmap_bloque);
	return dir_bitmap_bloque;

}

void leerBitmapDeBloque(uint32_t nroGrupo){

	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
//	uint16_t bloques_libres = grupo->free_blocks_count;
//	printf("Bloques libre: %hu\n",bloques_libres);

	uint32_t nroBloque = grupo->block_bitmap;
	uint8_t *posActual = ptr_arch + nroBloque * tamanio_bloque;

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->blocks_per_group;
//	printf("Bloques por grupo: %d\n",cantBloques);
	int cantBytes = cantBloques / 8;

	int bInicioDeGrupo = nroBloqueInicioDeGrupo(nroGrupo);

	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);
//	printf("set: %d\n",bitarray_test_bit(ptrBit,8191));

	int i;
	int blibres = 0;
	int bocupados = 0;
	for(i = 0;i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		printf("bloque %d: %d\n",i+bInicioDeGrupo,valor);
		if(valor == 0){
			printf("nroBloque %d: %d\n",i+bInicioDeGrupo,valor);
			blibres++;
		} else{
			bocupados++;
		}
	}
//	printf("blibres: %d\n",blibres);
//	printf("bocupados: %d\n",bocupados);
//	printf("Bloques libre: %hu\n",grupo->free_blocks_count);
//	printf("Bloques por grupo: %d\n",cantBloques);

}

void leerBitmapDeInodos(uint32_t nroGrupo){

//	struct CampoBits * cb;
	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
//	uint16_t inodos_libres = grupo->free_inodes_count;
//	printf("Bloques libre: %hu\n",bloques_libres);

	uint32_t nroBloque = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + nroBloque * tamanio_bloque;

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->inodes_per_group;
//	printf("Bloques por grupo: %d\n",cantBloques);
	int cantBytes = cantBloques / 8;

	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);

	int i;
	int iLibres = 0;
	int iOcupados = 0;
	for(i = 0;i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		printf("inodo %d: %d\n",i,valor);
		if(valor == 0){
			iLibres++;
		} else
			iOcupados++;
	}
//	printf("iLibres: %d\n",iLibres);
//	printf("iOcupados: %d\n",iOcupados);
//	printf("Inodos libre: %hu\n",inodos_libres);
//	printf("Inodos por grupo: %d\n",cantBloques);

}

void leerTablaDeInodos(uint32_t nroGrupo){
	struct Superblock *bloque = read_superblock();
	uint32_t cantInodos = bloque->inodes_per_group;
	printf("cantidad de inodos %d\n",cantInodos);

	uint8_t tamanioInodo = sizeof(struct INode);
	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
	uint8_t nroBloque = grupo->inode_table;

	printf("inodos libre: %u\n",grupo->free_inodes_count);

	uint8_t *dir_tabla_inodos = ptr_arch + nroBloque * tamanio_bloque;
	struct INode* inodo;
	int i;
	for(i=1;i<=cantInodos;i++){
		inodo = (struct INode*) dir_tabla_inodos;

		printf("\ninodo nro %d ",i);
		printf("mode: %hu ",inodo->mode);
		printf("uid: %hu ",inodo->uid);
		printf("atime: %u ",inodo->atime);
//		for(j = 0;j < 8; j++)
//			printf("block[%d] %u",j,inodo->blocks[j]);

		dir_tabla_inodos += tamanioInodo;
	}

}

t_queue * buscarBloquesLibres(uint32_t bloquesRequeridos){
//	struct Superblock *sb = read_superblock();
//	if(sb->free_blocks < bloquesRequeridos)
//		goto errorInsuficientesBloques;

	t_queue * bloquesLibres = queue_create();
	uint32_t nro_grupo;
	for(nro_grupo = 0;queue_size(bloquesLibres) < bloquesRequeridos;nro_grupo++)
		buscarBloquesLibresBitmaps(bloquesLibres,nro_grupo,bloquesRequeridos);
	return bloquesLibres;

}

void buscarBloquesLibresBitmaps(t_queue * bloquesLibres,uint32_t nro_grupo,uint32_t bloquesRequeridos){

	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioBB = grupo->block_bitmap;
	uint8_t *posActual = ptr_arch + inicioBB * tamanio_bloque;

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->blocks_per_group;
	int cantBytes = cantBloques / 8;
	uint32_t nroPrimerBloqueDelGrupo;

	 nroPrimerBloqueDelGrupo =  nroBloqueInicioDeGrupo(nro_grupo);

	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);

//	printf("primer bloque del grupo: %hu\n",nroPrimerBloqueDelGrupo);
	int i;
	for(i = 0;queue_size(bloquesLibres) < bloquesRequeridos && i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		if(valor == 0){
			uint32_t nroBloque = nroPrimerBloqueDelGrupo + i;
			queue_push(bloquesLibres,(void *)(nroBloque));
			printf("nroBloque agregado a la cola: %hu\n",nroBloque);
		}
//		printf("tamano cola bloques libres: %d\n",queue_size(bloquesLibres));
	}

}

t_queue * buscarInodosLibres(uint32_t inodosRequeridos){
	//	struct Superblock *sb = read_superblock();
	//	if(sb->free_blocks < bloquesRequeridos)
	//		goto errorInsuficientesBloques;

		t_queue * inodosLibres = queue_create();
		uint32_t nro_grupo;
		for(nro_grupo = 3;queue_size(inodosLibres) < inodosRequeridos;nro_grupo++)
			buscarInodosLibresBitmaps(inodosLibres,nro_grupo,inodosRequeridos);
		return inodosLibres;

}

void buscarInodosLibresBitmaps(t_queue * inodosLibres,uint32_t nro_grupo,uint32_t inodosRequeridos){

	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioIB = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + inicioIB * tamanio_bloque;

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->inodes_per_group;
	int cantBytes = cantBloques / 8;
	uint32_t nroPrimerInodoDelGrupo;

	nroPrimerInodoDelGrupo =  nroInodoInicioDeGrupo(nro_grupo);

	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);

//	printf("primer bloque del grupo: %hu\n",nroPrimerBloqueDelGrupo);
	int i;
	for(i = 0;queue_size(inodosLibres) < inodosRequeridos && i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		if(valor == 0){
			uint32_t nroInodo = nroPrimerInodoDelGrupo + i;
			queue_push(inodosLibres,(void *)(nroInodo));
			printf("nroInodo agregado a la cola: %hu\n",nroInodo);
		}
//		printf("tamano cola Inodos libres: %d\n",queue_size(inodosLibres));
	}

}

int nroBloqueInicioDeGrupo(uint32_t nro_grupo){
	struct Superblock *sb = read_superblock();
	uint32_t bloquesPorGrupo = sb->blocks_per_group;
//	printf("bloques por grupo: %hu\n",bloquesPorGrupo);
//	printf("inodos por grupo: %hu\n",bloque->inodes_per_group);
	int nroBloque;
	nroBloque = (tamanio_bloque == 1024) ? 1 : 0;
	return nroBloque + bloquesPorGrupo * nro_grupo;
}

int nroInodoInicioDeGrupo(uint32_t nro_grupo){
	struct Superblock *sb = read_superblock();
	uint32_t inodosPorGrupo = sb->inodes_per_group;
	return inodosPorGrupo * nro_grupo + 1; //el + 1 es porque empieza en el inodo nro 1
}

struct INode * getInodo(int nroInodo){

	struct Superblock *sb = read_superblock();
	int nroGrupo = (nroInodo - 1) / sb->inodes_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroBloqueTablaInodos = grupo->inode_table;
	uint8_t *dir_tabla_inodos = ptr_arch + nroBloqueTablaInodos * tamanio_bloque;
	uint8_t tamanioInodo = sizeof(struct INode);

	int cantInodos = nroInodo - nroGrupo * sb->inodes_per_group;

	struct INode* inodo;
	int i;
	for(i=1;i<cantInodos;i++,dir_tabla_inodos += tamanioInodo);
	inodo = (struct INode*) dir_tabla_inodos;
	return inodo;

}

uint8_t * posicionarInicioBloque(uint32_t nroBloque){
	return ptr_arch + nroBloque * tamanio_bloque;
}

void leerIndireccionSimple(uint32_t nroInodo){
	struct Superblock *sb = read_superblock();
	uint32_t tamanio_inodo = sb->inode_size;

	struct INode * inodo = getInodo(nroInodo);
	uint32_t bloqueIndireccionSimple = inodo->iblock;
	uint8_t * inicio = posicionarInicioBloque(bloqueIndireccionSimple);
//	printf("tamanio inodo: %d\n",tamanio_inodo);
	uint32_t cantidadPunteros = tamanio_bloque / tamanio_inodo;
//	printf("indireccion simple: %d\n",bloqueIndireccionSimple);
	uint32_t nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;){
		nroBloque = *(inicio += i * 4);
		printf("nro de bloque: %hu\n",nroBloque);
	}

}

void leerIndireccionDoble(uint32_t nroInodo){

	struct Superblock *sb = read_superblock();
	uint32_t tamanio_inodo = sb->inode_size;

	struct INode * inodo = getInodo(nroInodo);
	uint32_t bloqueIndireccionDoble = inodo->iiblock;
	uint8_t * inicio = posicionarInicioBloque(bloqueIndireccionDoble);
//	printf("tamanio inodo: %d\n",tamanio_inodo);
	uint32_t cantidadPunteros = tamanio_bloque / tamanio_inodo;
//	printf("indireccion simple: %d\n",bloqueIndireccionSimple);
	uint32_t nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;){
		nroBloque = *(inicio += i * 4);
		leerIndireccionSimple(nroBloque);
	}

}

void leerIndireccionTriple(uint32_t nroInodo){

	struct Superblock *sb = read_superblock();
	uint32_t tamanio_inodo = sb->inode_size;

	struct INode * inodo = getInodo(nroInodo);
	uint32_t bloqueIndireccionTriple = inodo->iiiblock;
	uint8_t * inicio = posicionarInicioBloque(bloqueIndireccionTriple);
//	printf("tamanio inodo: %d\n",tamanio_inodo);
	uint32_t cantidadPunteros = tamanio_bloque / tamanio_inodo;
//	printf("indireccion simple: %d\n",bloqueIndireccionSimple);
	uint32_t nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;){
		nroBloque = *(inicio += i * 4);
		leerIndireccionDoble(nroBloque);
	}

}
