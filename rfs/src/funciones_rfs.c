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
#include <stdbool.h>
//#include <commons/bitarray.h>
#include <src/commons/collections/queue.h>
//#include <commons/collections/queue.h>
#include <src/commons/collections/list.h>
#include <src/commons/string.h>

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
	struct Superblock *sb;
	uint8_t *posicion = ptr_arch + OFFSET;
	sb = (struct Superblock *) posicion;

//	printf("inodos= %d, \n",sb->inodes);
//	printf("inodos por grupo= %d, \n",sb->inodes_per_group);
//	printf("bloques= %d, \n",sb->blocks);
//	printf("bloques libres= %d, \n",sb->free_blocks);
//	printf("inodos libres= %d, \n",sb->free_inodes);
//	printf("bloques por grupo= %d, \n",sb->blocks_per_group);
//	printf("primer bloque de datos= %d, \n",sb->first_data_block);
//	printf("bloques reservados= %d, \n",sb->reserved_blocks);

	return sb;

}

// todo: darle uso
uint32_t cantidadDeGrupos() {
	struct Superblock *sb= read_superblock();
	uint32_t inodos = sb->inodes;
	uint32_t inodosPorGrupo = sb->inodes_per_group;
	double resto = inodos % inodosPorGrupo;
	int cantidadDeGrupos = inodos / inodosPorGrupo;
	return (resto != 0) ? cantidadDeGrupos + 1 : cantidadDeGrupos;
}

//void leerLosGroupDescriptor(uint32_t inodosTotales,uint32_t inodosPorGrupo) {
//
//	uint32_t cantDeGrupos = cantidadDeGrupos(inodosTotales,inodosPorGrupo);
//
//	struct GroupDesc * grupo;
//
//	uint16_t posInicialGD = 2048;
//	int32_t tamanioGrupo = sizeof(struct GroupDesc);
//	printf("Bytes de un grupo: %d\n", tamanioGrupo);
//	uint8_t *posActual;
//
//	uint32_t i;
//		posActual = ptr_arch+posInicialGD;
//		for(i=0;i < cantDeGrupos;i++) {
//
//		grupo = (struct GroupDesc*) posActual;
//
//		printf("Group Descriptor Nro %d\n",i);
//		printf("block_bitmap: %d\n",grupo->block_bitmap);
//		printf("inode_bitmap: %d\n",grupo->inode_bitmap);
//		printf("inode_table: %d\n",grupo->inode_table);
//		printf("free_blocks_count: %d\n",grupo->free_blocks_count);
//		printf("free_inodes_count: %d\n",grupo->free_inodes_count);
//		printf("used_dirs_count: %d\n\n",grupo->used_dirs_count);
//
//		posActual += tamanioGrupo;
//
//		}
//}

// todo: modificar la asignación a posInicialGD
struct GroupDesc * leerGroupDescriptor(uint32_t nroGrupo){
	struct GroupDesc *gd;
	uint16_t posInicialGD = 2048;
	uint8_t tamanioGrupo = sizeof(struct GroupDesc);
	uint8_t *posActual = ptr_arch + posInicialGD + (tamanioGrupo * nroGrupo);
	gd = (struct GroupDesc*) posActual;

//	printf("block_bitmap: %d\n",gd->block_bitmap);
//	printf("inode_bitmap: %d\n",gd->inode_bitmap);
//	printf("inode_table: %d\n",gd->inode_table);
//	printf("free_blocks_count: %d\n",gd->free_blocks_count);
//	printf("free_inodes_count: %d\n",gd->free_inodes_count);
//	printf("used_dirs_count: %d\n\n",gd->used_dirs_count);

	return gd;
}

//uint8_t *dir_inicioDeGrupo(uint32_t grupo) {
//
//	struct GroupDesc *TDgrupo = leerGroupDescriptor(grupo);
//	uint8_t *dir_bitmap_bloque = ptr_arch + (TDgrupo->block_bitmap * tamanio_bloque);
//	printf("direccion del grupo 1= %p, \n",dir_bitmap_bloque);
//	return dir_bitmap_bloque;
//
//}

void leerBitmapDeBloque(uint32_t nroGrupo){

	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);

	uint32_t nroBloque = grupo->block_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroBloque);

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->blocks_per_group;
	int cantBytes = cantBloques / 8;

	int bInicioDeGrupo = nroBloqueInicioDeGrupo(nroGrupo);

	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);

	int i;
//	int blibres = 0;
//	int bocupados = 0;
	for(i = 0;i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		printf("bloque %d: %d\n",i+bInicioDeGrupo,valor);
//		if(valor == 0){
//			printf("nroBloque %d: %d\n",i+bInicioDeGrupo,valor);
//			blibres++;
//		} else{
//			printf("nroBloque %d: %d\n",i+bInicioDeGrupo,valor);
//			bocupados++;
//		}
	}

}

void leerBitmapDeInodos(uint32_t nroGrupo){

	struct GroupDesc * grupo= leerGroupDescriptor(nroGrupo);
	uint32_t nroBloque = grupo->inode_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroBloque);

	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->inodes_per_group;
	int cantBytes = cantBloques / 8;

	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);

	int i;
//	int iLibres = 0;
//	int iOcupados = 0;
	for(i = 0;i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		printf("inodo %d: %d\n",i,valor);
//		if(valor == 0){
//			iLibres++;
//		} else
//			iOcupados++;
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
		for(nro_grupo = 0;queue_size(inodosLibres) < inodosRequeridos;nro_grupo++)
			buscarInodosLibresBitmaps(inodosLibres,nro_grupo,inodosRequeridos);
		return inodosLibres;

}

void buscarInodosLibresBitmaps(t_queue * inodosLibres,uint32_t nro_grupo,uint32_t inodosRequeridos){

	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioIB = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + inicioIB * tamanio_bloque;

	struct Superblock *sb = read_superblock();
	int cantInodos = sb->inodes_per_group;
	int cantBytes = cantInodos / 8;
	uint32_t nroPrimerInodoDelGrupo;

	nroPrimerInodoDelGrupo =  nroInodoInicioDeGrupo(nro_grupo);

	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);

//	printf("primer bloque del grupo: %hu\n",nroPrimerBloqueDelGrupo);
	int i;
	for(i = 0;queue_size(inodosLibres) < inodosRequeridos && i < cantInodos;i++){
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

struct INode * getInodo(uint32_t nroInodo){

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

void leerBloquesInodos(struct INode * inodo){

	leerBloquesDirectos(inodo);
	leerIndireccionSimple(inodo->iblock);
//	leerIndireccionDoble(inodo->iiblock);
//	leerIndireccionTriple(inodo->iiiblock);

}

void leerBloquesDirectos(struct INode * inodo){
	int i;
	for(i = 0;i < 12;i++)
		printf("bloque directo: %d %u\n",i+1,inodo->blocks[i]);
}

void leerIndireccionSimple(uint32_t iblock){

	uint8_t * inicio = posicionarInicioBloque(iblock);
	uint32_t cantidadPunteros = tamanio_bloque / sizeof(uint32_t);
	uint32_t * nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;i++){
		nroBloque = (uint32_t *)(inicio + i * 4);
		printf("nro de bloque: %u\n",*nroBloque);
	}

}

void leerIndireccionDoble(uint32_t iiblock){

	uint8_t * inicio = posicionarInicioBloque(iiblock);
	uint32_t cantidadPunteros = tamanio_bloque / sizeof(uint32_t);
	uint32_t * nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;i++){
		nroBloque = (uint32_t *)(inicio + i * 4);
		leerIndireccionSimple(*nroBloque);
	}

}

void leerIndireccionTriple(uint32_t iiiblock){

	uint8_t * inicio = posicionarInicioBloque(iiiblock);
	uint32_t cantidadPunteros = tamanio_bloque / sizeof(uint32_t);
	uint32_t * nroBloque;
	int i;
	for(i = 0;i < cantidadPunteros;i++){
		nroBloque = (uint32_t *)(inicio + i * 4);
		leerIndireccionDoble(*nroBloque);
	}

}

void leerDirectorio(char * path){
//	leerBloquesDeDatosHasta(inodoArchivo->size);
//	t_list * listaStats;
	char * separador = "/";
	char ** ruta_separada = string_split(path,separador);
	int tamanio_ruta_separada = size_array_before_split(path,separador);
	struct INode * inodoDeBusqueda = getInodo(2); // el inodo 2 es el directorio raíz
	int i;
	for(i = 0;i < tamanio_ruta_separada;i++){
		inodoDeBusqueda = buscarInodoQueGestiona(inodoDeBusqueda,ruta_separada[i]);
	}
	// todo: cargar en las estructuras
//	return listaStats = cargarEntradasDirectorioALista(inodoDeBusqueda);
}

struct INode * buscarInodoQueGestiona(struct INode * inodoDeBusqueda,char * ruta){
//	leerBloquesDeDatosHasta(inodoArchivo->size);
	struct INode * inodo;
	uint32_t nroBloque = inodoDeBusqueda->blocks[0];
	uint8_t * inicio = posicionarInicioBloque(nroBloque);
	uint16_t cantidad = 0;
	int i = 0;
	while(tamanio_bloque > cantidad){
		printf("entrada de directorio %d\n",i++);
		struct DirEntry * directorio = (struct DirEntry *) inicio;
		printf("nro inodo: %u\n",directorio->inode);
		printf("entry_len: %u\n",directorio->entry_len);
		printf("name_len: %u\n",directorio->name_len);
		printf("type: %u\n",directorio->type);
		char* nombre = calloc(1, directorio->name_len + 1);
		memcpy(nombre, directorio->name, directorio->name_len);
		printf("name: %s\n", nombre);
		if(string_equals_ignore_case(nombre,ruta)){
			printf("nro de inodo que gestiona %s es %u\n",ruta,directorio->inode);
			inodo = getInodo(directorio->inode);
			break;
		}
		cantidad += directorio->entry_len;
		inicio += directorio->entry_len;
	}

	return inodo;
}

void leerArchivo(char * path){
	char * separador = "/";
	char ** ruta_separada = string_split(path,separador);
	int tamanio_ruta_separada = size_array_before_split(path,separador);
	struct INode * inodoDeBusqueda = getInodo(2); // el inodo 2 es el directorio raíz
	int i;
	for(i = 0;i < tamanio_ruta_separada;i++){
		inodoDeBusqueda = buscarInodoQueGestiona(inodoDeBusqueda,ruta_separada[i]);
	}
//	return listaStats = cargarEntradasDirectorioALista(inodoDeBusqueda);
}

//bool esInodoDirectorio(uint32_t nroInodo){
//	struct INode * inodo = getInodo(nroInodo);
//	return EXT2_INODE_HAS_MODE_FLAG(inodo, 0x4000);
//}


// todo: deprecated

//t_queue * buscarDirectorios(){
//
//	struct Superblock *sb = read_superblock();
//	uint32_t cantGrupos = cantidadDeGrupos(sb->inodes,sb->inodes_per_group);
//
//	t_queue * inodosDirectorios = queue_create();
//
//	uint32_t nro_grupo;
//
//	for(nro_grupo = 0;nro_grupo < cantGrupos;nro_grupo++){
//		struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
//		uint32_t inicioIB = grupo->inode_bitmap;
//		uint8_t *posActual = ptr_arch + inicioIB * tamanio_bloque;
//
//		int cantInodos = sb->inodes_per_group;
//		int cantBytes = cantInodos / 8;
//		uint32_t nroPrimerInodoDelGrupo;
//
//		nroPrimerInodoDelGrupo =  nroInodoInicioDeGrupo(nro_grupo);
//
//		t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);
//
//	//	printf("primer bloque del grupo: %hu\n",nroPrimerBloqueDelGrupo);
//		int i;
//		for(i = 0;i < cantInodos;i++){
//			bool valor = bitarray_test_bit(ptrBit, i);
//			if(valor == 1){
//				uint32_t nroInodo = nroPrimerInodoDelGrupo + i;
//				struct INode * inodo = getInodo(nroInodo);
//				//todo : hacer define con cada tipo de mode
//				if (EXT2_INODE_HAS_MODE_FLAG(inodo, 0x4000)){	//0x4000 directorio
//					printf("es directorio el inodo: %d\n",i);
//					queue_push(inodosDirectorios,(void *)(nroInodo));
//				}
//			}
//		}
//	}
//	return inodosDirectorios;
//}

// todo: deprecated
//t_queue * buscarDirectorios(){
//	//	struct Superblock *sb = read_superblock();
//	//	if(sb->free_blocks < bloquesRequeridos)
//	//		goto errorInsuficientesBloques;
//
//		t_queue * inodos = queue_create();
//		uint32_t nro_grupo;
//		uint32_t cantidadDeGrupos = 4;
//		for(nro_grupo = 0;nro_grupo < cantidadDeGrupos;nro_grupo++)
//			buscarInodosDirectorio(inodos,nro_grupo);
//		return inodos;
//
//}

// todo: deprecated
//void buscarInodosDirectorio(t_queue * inodos,uint32_t nro_grupo){
//
//	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
//	uint32_t inicioIB = grupo->inode_bitmap;
//	uint8_t *posActual = ptr_arch + inicioIB * tamanio_bloque;
//
//	struct Superblock *sb = read_superblock();
//	int cantInodos = sb->inodes_per_group;
//	int cantBytes = cantInodos / 8;
//	uint32_t nroPrimerInodoDelGrupo;
//
//	nroPrimerInodoDelGrupo =  nroInodoInicioDeGrupo(nro_grupo);
//
//	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);
//
////	printf("primer bloque del grupo: %hu\n",nroPrimerBloqueDelGrupo);
//	int i;
//	for(i = 0;i < cantInodos;i++){
//		bool valor = bitarray_test_bit(ptrBit, i);
//		if(valor == 1){
//			uint32_t nroInodo = nroPrimerInodoDelGrupo + i;
//			struct INode * unInodo = getInodo(nroInodo);
//			if(EXT2_INODE_HAS_MODE_FLAG(unInodo,0x4000)){
//				queue_push(inodos,(void *)nroInodo);
//				printf("nroInodo agregado a la cola: %hu\n",nroInodo);
//			}
//		}
//	}
//
//}


//void leerArchivo(char * path){
//
////	leerDirectorioRaiz();
//	uint32_t nroInodo = 2;
//	char * separador = "/";
//	char ** vector = string_split(path,separador);
//	int size = size_array_before_split(path,separador);
//	int i;
//	for(i = 0;i < size;i++) {
//		char * unDir = vector[i];
//		nroInodo = buscarInodoEnDirectorio(nroInodo,unDir);
//	}
//	struct INode * inodoArchivo = getInodo(nroInodo);
//	recorrerArchivo(inodoArchivo);
//
//}

//uint32_t buscarInodoEnDirectorio(uint32_t nroInodo,char * unDir){
//	struct INode * inodo = getInodo(nroInodo);
//	if (!EXT2_INODE_HAS_MODE_FLAG(inodo, 0x4000)){	//0x4000 directorio
//		printf("el inodo no es directorio, error\n");
//	}
//	uint32_t nroInodoQueManejaArchivo = compararConEntradasDirectorio(inodo,unDir);
//	return nroInodoQueManejaArchivo;
//}
//
//uint32_t compararConEntradasDirectorio(struct INode * inodo,char * unDir){
////	leerBloquesDeDatosHasta(inodo->size);
//	uint32_t nroInodoQueManejaArchivo;
//	uint32_t nroBloque = inodo->blocks[0];
//	uint8_t * inicio = posicionarInicioBloque(nroBloque);
//	uint16_t cantidad = 0;
//	int i = 0;
//	while(tamanio_bloque > cantidad){
//		printf("entrada de directorio %d\n",i++);
//		struct DirEntry * directorio = (struct DirEntry *) inicio;
//		printf("nro inodo: %u\n",directorio->inode);
//		printf("entry_len: %u\n",directorio->entry_len);
//		printf("name_len: %u\n",directorio->name_len);
////		printf("type: %u\n",directorio->type);
//		printf("name: %s\n",directorio->name);
//		if(string_equals_ignore_case(unDir,directorio->name)){
//			printf("el inodo que lo maneja es: %d\n",nroInodoQueManejaArchivo);
//			nroInodoQueManejaArchivo = directorio->inode;
//		}
//		cantidad += directorio->entry_len;
//		inicio += directorio->entry_len;
//	}
//	return nroInodoQueManejaArchivo;
//}
//
//void recorrerArchivo(struct INode * inodoArchivo){
//	leerBloquesDeDatosHasta(inodoArchivo->size);
//}

//void leerDirectorio(struct INode * inodo){
////	leerBloquesDeDatosHasta(inodoArchivo->size);
//	uint32_t nroBloque = inodo->blocks[0];
//	uint8_t * inicio = posicionarInicioBloque(nroBloque);
//	uint16_t cantidad = 0;
//	int i = 0;
//	while(tamanio_bloque > cantidad){
//		printf("entrada de directorio %d\n",i++);
//		struct DirEntry * directorio = (struct DirEntry *) inicio;
//		printf("nro inodo: %u\n",directorio->inode);
//		printf("entry_len: %u\n",directorio->entry_len);
//		printf("name_len: %u\n",directorio->name_len);
////		printf("type: %u\n",directorio->type);
//		printf("name: %s\n",directorio->name);
//		cantidad += directorio->entry_len;
//		inicio += directorio->entry_len;
//	}
//
//}
