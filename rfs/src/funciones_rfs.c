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
#include <src/commons/config.h>

#define PATH "/home/utnso/Desarrollo/ext2.disk"
#define BLOQUE 1024
//#define PATH_CONFIG "home/utnso/archivo_configuracion";

static const int tamanio_bloque = 1024;
//uint32_t *ptr_arch;
uint8_t *ptr_arch;

void mapear_archivo() {
	// todo: revisar el archivo de configuracion
//	t_config * config = config_create(PATH_CONFIG);
//	char * path_a = config_get_string_value(config,"path_disco");
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

// todo: modificar la asignación a posInicialGD de acuerdo al tamaño de bloque
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

	printf("nroInodo: %hu\n",nroInodo);
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

//t_list * listarDirectorio(char * path){
//	struct INode * inodoDeBusqueda = getInodoDeLaDireccionDelPath(path);
//	// todo: cargar en las estructuras
//	return cargarEntradasDirectorioALista(inodoDeBusqueda);
//
//}

// de prueba todo: controlar que el path es un directorio
void listarDirectorio(char * path){
	struct INode * inodoDeBusqueda = getInodoDeLaDireccionDelPath(path);
	cargarEntradasDirectorioALista(inodoDeBusqueda);
}

uint32_t buscarNroInodoEnEntradasDirectorio(struct INode * inodoDeBusqueda,char * ruta){

	uint32_t nroInodoBuscado = 0;
	uint32_t tamanio_directorio = inodoDeBusqueda->size;
	uint32_t offset;
	for(offset = 0;nroInodoBuscado == 0 && offset < tamanio_directorio;offset += tamanio_bloque){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodoDeBusqueda,nroBloqueLogico,0);

		uint16_t cantidad = 0;
		int i = 0;
		while(tamanio_bloque > cantidad){
			printf("entrada de directorio %d\n",i++);
			struct DirEntry * directorio = (struct DirEntry *) ptr;
			printf("nro inodo: %u\n",directorio->inode);
			printf("entry_len: %u\n",directorio->entry_len);
			printf("name_len: %u\n",directorio->name_len);
			printf("type: %u\n",directorio->type);
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			printf("name: %s\n", nombre);
			if(string_equals_ignore_case(nombre,ruta)){
				printf("nro de inodo que gestiona %s es %u\n",ruta,directorio->inode);
//				inodoBuscado = getInodo(directorio->inode);
				nroInodoBuscado = directorio->inode;
				break;
			}
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}

	}

	return nroInodoBuscado;
}


// de prueba
void cargarEntradasDirectorioALista(struct INode * inodo){

	uint32_t tamanio_directorio = inodo->size;
	uint32_t offset;
	for(offset = 0;offset < tamanio_directorio;offset += tamanio_bloque){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodo,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanio_bloque > cantidad){
			// todo: ver si carga bien los datos a la estructura
			struct DirEntry * directorio = (struct DirEntry *) ptr;
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
//			inodoEntradaDirectorioActual = getInodo(directorio->inode);
			printf("nombre: %s\n",nombre);
			printf("entry_len: %d\n",directorio->entry_len);
			printf("inode: %d\n",directorio->inode);
//			printf("type: %hu\n",directorio->type);
			printf("name_len: %hu\n\n",directorio->name_len);
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}
	}
}


//t_list * cargarEntradasDirectorioALista(struct INode * inodo){
//	t_list * lista = list_create();
//
//	struct INode * inodoEntradaDirectorioActual;
//	uint32_t tamanio_directorio = inodo->size;
//	uint32_t offset;
//	for(offset = 0;offset < tamanio_directorio;offset += tamanio_bloque){
//		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
//		void * ptr = posicionarme(inodo,nroBloqueLogico,0);
//		uint16_t cantidad = 0;
//		while(tamanio_bloque > cantidad){
//			// todo: ver si carga bien los datos a la estructura
//			struct DirEntry * directorio = (struct DirEntry *) ptr;
//			printf("nombre %s\n",directorio->name);
//			char* nombre = calloc(1, directorio->name_len + 1);
//			memcpy(nombre, directorio->name, directorio->name_len);
//			inodoEntradaDirectorioActual = getInodo(directorio->inode);
//			t_readdir_stat * readdirStats = malloc(sizeof(t_readdir_stat));
//			readdirStats->mode = inodoEntradaDirectorioActual->mode;
//			readdirStats->gid = inodoEntradaDirectorioActual->gid;
//			readdirStats->nlink = inodoEntradaDirectorioActual->links;
//			readdirStats->uid = inodoEntradaDirectorioActual->uid;
//			readdirStats->name = nombre;
//			list_add(lista,inodoEntradaDirectorioActual);
//
//			cantidad += directorio->entry_len;
//			ptr += directorio->entry_len;
//		}
//
//	}
//	return lista;
//}

void leerArchivo(char * path,uint32_t offset,uint32_t size){

	uint32_t nroInodoDeBusqueda = getNroInodoDeLaDireccionDelPath(path);
	if(nroInodoDeBusqueda == 0){
		printf("no existe el archivo");
	} else {
		struct INode * inodoDeBusqueda = getInodoDeLaDireccionDelPath(path);
		if((size <= 0) || (inodoDeBusqueda->size <= offset + size))
			perror("no es posible realizar la lectura");
		else
			guardarDatosArchivos(inodoDeBusqueda,offset,size);
	}
}

void guardarDatosArchivos(struct INode * inodo,uint32_t offset,uint32_t size){
	while(size > 0){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		uint32_t desplazamiento = desplazamientoDentroDelBloque(offset);
		void * ptr = posicionarme(inodo,nroBloqueLogico,desplazamiento);
		char* string_archivo;
		printf("el archivo contiene\n");

		if(size + desplazamiento > tamanio_bloque){
			uint32_t resto = tamanio_bloque - desplazamiento;
			string_archivo = calloc(1, resto + 1);
			memcpy(string_archivo, ptr, resto);
			size -= resto;
			offset += resto;
		} else {
			string_archivo = calloc(1, size + 1);
			memcpy(string_archivo, ptr, size);
			offset += size;
			size = 0;
		}
		printf("%s\n",string_archivo);
	}
}

uint32_t nroBloqueDentroDelInodo(uint32_t offset){
	return offset / tamanio_bloque;
}

uint32_t desplazamientoDentroDelBloque(uint32_t offset){
	return offset % tamanio_bloque;
}

int esBloqueDirecto(uint32_t nroBloque) {
	return nroBloque < 12;
}

int esIndireccionSimple(uint32_t nroBloqueLogico) {
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t);
	return (12 <= nroBloqueLogico) && (nroBloqueLogico < 12 + cantBloquesPorIndireccion);
//	return nroBloqueLogico < 12 + cantBloquesPorIndireccion;
}

uint32_t * posicionarIndireccionSimple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico) {
	uint8_t* inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t* ptrBloque;
	int i;
	uint32_t cantBloquesYaRecorridos = 12;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	for (i = 0; i < nroBloqueLogico; i++);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	printf("nro de bloque: %u\n", *ptrBloque);
	return ptrBloque;
}

int esIndireccionDoble(uint32_t nroBloqueLogico){
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t);
	return (12 + cantBloquesPorIndireccion <= nroBloqueLogico) && (nroBloqueLogico < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2));
//	return nroBloqueLogico < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2);
}

uint32_t * posicionarIndireccionDoble(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanio_bloque / sizeof(uint32_t);
	int i;
	// puede romper aca, sino probar con 268
	uint32_t cantBloquesYaRecorridos = 267;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	for (i = 0; cantBloquesPorIndireccionSimple <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionSimple);
	ptrBloque = (uint32_t*) (inicio + i * 4);	// me ubico en la indireccion doble que contiene el bloque logico que busco
	inicio = posicionarInicioBloque(*ptrBloque);
	ptrBloque = (uint32_t *) (inicio + nroBloqueLogico * 4); // dentro de la indireccion doble, me ubico en la indireccion simple
	return ptrBloque;
}

int esIndireccionTriple(uint32_t nroBloque){
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t);
	return (12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2) <= nroBloque) && (nroBloque < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2)
			+ pow(cantBloquesPorIndireccion,3));
//	return nroBloque < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2)
//			+ pow(cantBloquesPorIndireccion,3);

}

uint32_t * posicionarIndireccionTriple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanio_bloque / sizeof(uint32_t);
	uint32_t cantBloquesPorIndireccionDoble = pow(cantBloquesPorIndireccionSimple,2);
	uint32_t cantBloquesYaRecorridos = 65803;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	int i,j;
	for (i = 0; cantBloquesPorIndireccionDoble <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionDoble);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	for (j = 0; cantBloquesPorIndireccionSimple <= nroBloqueLogico; j++, nroBloqueLogico -= cantBloquesPorIndireccionSimple);
	ptrBloque = (uint32_t*) (inicio + j * 4);	// me ubico en la indireccion doble que contiene el bloque logico que busco
	inicio = posicionarInicioBloque(*ptrBloque);
	ptrBloque = (uint32_t *) (inicio + nroBloqueLogico * 4); // dentro de la indireccion doble, me ubico en la indireccion simple
	return ptrBloque;
}

/*
 * Me posiciono dentro del número lógico de bloque dentro del inodo y me desplazo dentro de éste
 */
void * posicionarme(struct INode * inodo,uint32_t nroBloqueLogico,uint32_t desplazamiento){
	void * ptr;
	uint32_t * ptrNroBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodo,nroBloqueLogico);
	printf("nroBloqueDeDato %u\n",*ptrNroBloqueLogico);
	ptr = desplazarme(*ptrNroBloqueLogico, desplazamiento);
	return ptr;
}

/*
 * Dentro de un bloque de dato, me desplazo para poder posicionarme correctamente
 */
void * desplazarme(uint32_t nroBloqueDeDato,uint32_t desplazamiento){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueDeDato);
	return inicio += desplazamiento;
}

/*
 * Desc: me devuelve el inodo que maneja la ultima ruta del path
 * Ej: si le paso path = "carpeta/archivo", me da el inodo que gestiona a 'archivo'
 */
struct INode * getInodoDeLaDireccionDelPath(char * path){
	char * separador = "/";
	char ** ruta_separada = string_split(path,separador);
	int tamanio_ruta_separada = size_array_before_split(path,separador);
	uint32_t nroInodoDeBusqueda = 2;	// el inodo 2 es el directorio raiz
	struct INode * inodoDeBusqueda = getInodo(nroInodoDeBusqueda);
	int i;
	for(i = 0;i < tamanio_ruta_separada;i++){
		nroInodoDeBusqueda = buscarNroInodoEnEntradasDirectorio(inodoDeBusqueda,ruta_separada[i]);
		if(nroInodoDeBusqueda == 0){
			printf("error: no existe la ruta");
			break;
		}
		inodoDeBusqueda = getInodo(nroInodoDeBusqueda);
	}
	return inodoDeBusqueda;
}

void truncarArchivo(char * path,uint32_t offset){
	if(offset < 0){
		perror("error: el offset debe ser >= 0");
	} else {
		struct INode * inodoPath = getInodoDeLaDireccionDelPath(path);
		uint32_t offsetEOF = inodoPath->size; // el -1 es para sacarle el EOF cuando trabajo con un archivo creado externamente
		int32_t size = offset - offsetEOF;
		if(size == 0)
			printf("no hay cambio");
		if(size < 0){
			if(offsetEOF < abs(size)) // si el size es negativo y es mayor en valor absoluto que el tamanio del archivo: error
				perror("truncamiento invalido");
			else{
				// achicar archivo
				while(size < 0){
					uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offsetEOF - 1);	//el -1 es por el EOF
					uint32_t desplazamiento = desplazamientoDentroDelBloque(offsetEOF - 1) + 1;	//para que cuando me den un offset multiplo de tamanio_bloque, el desplazamiento me devuelva tamanio_bloque
					if(desplazamiento <= abs(size)){
						uint32_t * ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoPath,nroBloqueLogico);
						liberarBloque(ptrBloqueLogico);
						offsetEOF -= desplazamiento;
						size += desplazamiento;
					} else {
						offsetEOF += size;
						size -= size;
					}
				}
				inodoPath->size = offsetEOF;
			}
		} else {
			//agrandar archivo
			// todo: controlar si hay suficientes espacio en disco
			while(size > 0){
				uint32_t resto = tamanio_bloque - desplazamientoDentroDelBloque(offsetEOF);
				if(resto == tamanio_bloque){
					uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offsetEOF);
					uint32_t nroBloqueDeDato = getBloqueLibre();

// manejo de la asignación de bloques para indirecciones simples, dobles y triples
					uint32_t cantPtrEnBloque = tamanio_bloque / sizeof(uint32_t);
					if(esIndireccionSimple(nroBloqueLogico) && nroBloqueLogico == 12)
							inodoPath->iblock = getBloqueLibre();
					if(esIndireccionDoble(nroBloqueLogico)){
						if(nroBloqueLogico == 268){
							inodoPath->iiblock = getBloqueLibre();
						}
						// el 12 es por los bloques que ya recorrio
						if((nroBloqueLogico - 12) % cantPtrEnBloque == 0){
							uint32_t posDentroIndireccionDoble = (nroBloqueLogico - 12) / cantPtrEnBloque;
							void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
							uint32_t * nroBloqueLibre = getBloqueLibre();
							memcpy(ptrBloque,nroBloqueLibre,sizeof(uint32_t));
						}
					}
					if(esIndireccionTriple(nroBloqueLogico)){
						if(nroBloqueLogico == 65804){
							inodoPath->iiiblock = getBloqueLibre();
						}
						if(nroBloqueLogico - (12 + cantPtrEnBloque) % cantPtrEnBloque == 0){
							uint32_t posDentroIndireccionDoble = (nroBloqueLogico - (12 + cantPtrEnBloque)) / cantPtrEnBloque;
							void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
							uint32_t * nroBloqueLibre = getBloqueLibre();
							memcpy(ptrBloque,nroBloqueLibre,sizeof(uint32_t));
						}
					}
// Fin - manejo de la asignación de bloques para indirecciones simples, dobles y triples

					agregarAInodo(inodoPath,nroBloqueLogico,nroBloqueDeDato);
				}
				if(resto >= size){
					completarBloque(inodoPath,offsetEOF,size);	// completar: se tiene que parar en el offset donde quiere escribir
					offsetEOF += size;
					size -= size;
				} else {
					completarBloque(inodoPath,offsetEOF,resto);
					offsetEOF += resto;
					size -= resto;
				}
			}
			inodoPath->size = offsetEOF;
		}
	}
}

/*
 * Desc: mediante el inodo y un número de bloque lógico, me devuelve un puntero al número
 * 		 de bloque lógico dentro del inodo
 */
uint32_t * getPtrNroBloqueLogicoDentroInodo(struct INode * inodo,uint32_t nroBloqueLogico){
	uint32_t * ptrNroBloqueLogico;
	if (esBloqueDirecto(nroBloqueLogico)){
		ptrNroBloqueLogico = &(inodo->blocks[nroBloqueLogico]);
	} else if (esIndireccionSimple(nroBloqueLogico)) {
		ptrNroBloqueLogico = posicionarIndireccionSimple(inodo->iblock, nroBloqueLogico);
	} else if (esIndireccionDoble(nroBloqueLogico)){
		ptrNroBloqueLogico = posicionarIndireccionDoble(inodo->iiblock, nroBloqueLogico);
	} else if (esIndireccionTriple(nroBloqueLogico)){
		ptrNroBloqueLogico = posicionarIndireccionTriple(inodo->iiiblock, nroBloqueLogico);
	}
	return ptrNroBloqueLogico;
}

/*
 * Libero el bloque de dato
 */
void liberarBloque(uint32_t * ptrBloqueDeDato){
	uint32_t nroBloque = 0;
	uint32_t * ptrNroBloque = &nroBloque;
	uint32_t nroBloqueDeDato = *ptrBloqueDeDato;
	struct Superblock *sb = read_superblock();
	//todo : ver si funca para todos los casos
	uint32_t nroGrupo = (nroBloqueDeDato - 1) / sb->blocks_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroBlockBitmap = grupo->block_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroBlockBitmap);
	int cantBytes = sb->blocks_per_group / 8;
	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);
	while(nroBloqueDeDato > sb->blocks_per_group)
		nroBloqueDeDato -= sb->blocks_per_group;
	//marco como libre el bloque en el bitmap de bloques
	bitarray_clean_bit(ptrBit,nroBloqueDeDato - 1); // el -1 es porque la función empieza a leer desde cero, pero el nroBloqueDeDatos puede ser >= 1
	// Pongo en cero al número de bloque relativo dentro del inodo
	memcpy(ptrBloqueDeDato,ptrNroBloque,sizeof(uint32_t));
	(grupo->free_blocks_count)++;	//aumento la cantidad de bloques libres en el group descriptor
	(sb->free_blocks)++;			//aumento la cantidad de bloques libres en el superbloque
}

uint32_t getBloqueLibre(){
	struct Superblock *sb = read_superblock();
	if(sb->free_blocks == 0)
		perror("error: No hay bloques disponibles");
	uint32_t nroBloqueDeDato = 0;
	uint32_t nro_grupo;
	for(nro_grupo = 0;nroBloqueDeDato == 0;nro_grupo++)
		nroBloqueDeDato = getBloqueLibreDelBitmap(nro_grupo);
	actualizarEstructurasCuandoPidoBloque(nroBloqueDeDato);
	return nroBloqueDeDato;
}

uint32_t getBloqueLibreDelBitmap(uint32_t nro_grupo){
	uint32_t nroBloqueDeDato = 0;
	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioBB = grupo->block_bitmap;
	uint8_t *posActual = ptr_arch + inicioBB * tamanio_bloque;
	struct Superblock *bloque = read_superblock();
	int cantBloques = bloque->blocks_per_group;
	int cantBytes = cantBloques / 8;
	uint32_t nroPrimerBloqueDelGrupo;
	nroPrimerBloqueDelGrupo =  nroBloqueInicioDeGrupo(nro_grupo);
	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);
	int i;
	for(i = 0;i < cantBloques;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		if(valor == 0){
			nroBloqueDeDato = nroPrimerBloqueDelGrupo + i;
			break;
		}
	}
	return nroBloqueDeDato;
}

void agregarAInodo(struct INode * inodoPath,uint32_t nroBloqueLogico,uint32_t nroBloqueDeDato){
	// me posiciono en el nro de bloque relativo al inodo
	uint32_t * ptrBloque = getPtrNroBloqueLogicoDentroInodo(inodoPath,nroBloqueLogico);
	// Le asigno al número de bloque relativo del inodo, el número de bloque de dato libre
	uint32_t * ptrNroBloqueDeDato = &nroBloqueDeDato;
	memcpy(ptrBloque,ptrNroBloqueDeDato,sizeof(uint32_t));
}

void completarBloque(struct INode * inodo, uint32_t offset, uint32_t size){
	uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
	uint32_t desplazamiento = desplazamientoDentroDelBloque(offset);
	void * ptr = posicionarme(inodo,nroBloqueLogico,desplazamiento);
	escribirBloque(ptr,size);
}

void escribirBloque(void * posicionPtr,uint32_t size){
	uint32_t i;
	char * dato = "";
	for(i = 0;i < size;i++,posicionPtr++)
		memcpy(posicionPtr,dato,1);
}

void escribirArchivo(char * path, char * input, uint32_t size, uint32_t offset){

	// todo: hacer un control si hay espacio en el disco
	uint32_t nroInodoPath = getNroInodoDeLaDireccionDelPath(path);
	if(nroInodoPath == 0){
		printf("no existe el archivo");
	} else{
		struct INode * inodoPath = getInodo(nroInodoPath);
		if(inodoPath->size < size + offset){
			uint32_t truncar = size + offset;
			truncarArchivo(path,truncar);
		}

		while(size > 0){
			uint32_t resto = tamanio_bloque - desplazamientoDentroDelBloque(offset);
			if(resto >= size){
				escribir(inodoPath,input,size,offset);
				offset += size;
				size -= size;
			} else {
				escribir(inodoPath,input,resto,offset);
				offset += resto;
				size -= resto;
			}
		}
	}

}

void escribir(struct INode * inodo, char * input,uint32_t size,uint32_t offset){
	uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
	uint32_t desplazamiento = desplazamientoDentroDelBloque(offset);
	void * ptr = posicionarme(inodo,nroBloqueLogico,desplazamiento);
	char * subString = malloc(size);
	strncpy(subString,input,size);
	input += size;
	memcpy(ptr,subString,size);
	free(subString);
}

// todo: ver manejo de errores, agregar el mode para asignarle al inodo de la nueva entrada
void crearDirectorio(char * path){
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	struct INode * inodoRuta = getInodo(nroInodoRuta);
	if(nroInodoRuta != 0)
		if(getNroInodoDeLaDireccionDelPath(path) == 0){
			agregarEntradaDirectorio(inodoRuta,ruta_separada->nombre);
			struct INode * inodoNuevoDir = getInodoDeLaDireccionDelPath(path);
			// nro de inodo que cargo para . en el nuevo directorio
			uint32_t nroInodoEntradaNueva = getNroInodoDeLaDireccionDelPath(path);
			// nro de inodo que cargo para .. en el nuevo directorio
			uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
			inicializarEntradasNuevoDir(inodoNuevoDir,0,nroInodoEntradaNueva,nroInodoRuta);
			free(ruta_separada->ruta);
			free(ruta_separada->nombre);
			free(ruta_separada);
		} else {
			printf("error: ya existe la carpeta %s",ruta_separada->nombre);
		}
	else
		printf("error: no existe la ruta");

}

t_ruta_separada * separarPathParaNewDirEntry(char * path){
	t_ruta_separada * ruta_separada = malloc(sizeof(t_ruta_separada));
	char * separador = "/";
	if(size_array_before_split(path,separador) == 1){
		char ** vector_ruta = string_split(path,separador);
		ruta_separada->ruta = calloc(1,1);
		ruta_separada->nombre = calloc(1,strlen(vector_ruta[0]));
		strncpy(ruta_separada->nombre,vector_ruta[0],strlen(vector_ruta[0]));
	} else {
		char * nombre_carpeta = rindex(path,'/');
		uint32_t tamanio_nombre = strlen(nombre_carpeta);
		ruta_separada->ruta = calloc(1,strlen(path) - tamanio_nombre + 1);
		strncpy(ruta_separada->ruta,path,strlen(path) - tamanio_nombre);
		nombre_carpeta++;
		ruta_separada->nombre = calloc(1,strlen(nombre_carpeta) + 1);
		strncpy(ruta_separada->nombre,nombre_carpeta,tamanio_nombre);
	}
	return ruta_separada;
}

/*
 * Agrega en las entradas de directorio de inodo, una entrada que puede ser del tipo archivo o directorio
 */
void agregarEntradaDirectorio(struct INode * inodo,char * nameNewEntry){

//	uint32_t nroInodoEntradaNueva;
	uint32_t tamanio_nueva_entrada = tamanioMinEntradaDirectorio(nameNewEntry);
	uint32_t tamanio_directorio = inodo->size;
	uint32_t offset;
	int encontroEspacio = 0;
	for(offset = 0;!encontroEspacio && offset < tamanio_directorio;offset += tamanio_bloque){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodo,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanio_bloque > cantidad){
			// todo: ver si carga bien los datos a la estructura
			struct DirEntry * directorio = (struct DirEntry *) ptr;
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			uint32_t sizeMinEntradaDirectorioActual = tamanioMinEntradaDirectorio(nombre);
			uint32_t resto = directorio->entry_len - sizeMinEntradaDirectorioActual;
			if(resto >= tamanio_nueva_entrada){
				directorio->entry_len = sizeMinEntradaDirectorioActual;
				void * ptrInicioNuevaEntrada = ptr + sizeMinEntradaDirectorioActual;
				agregarNuevaEntrada(ptrInicioNuevaEntrada,nameNewEntry,resto);
				encontroEspacio = 1;
				free(nombre);
				break;
			}
			free(nombre);
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}
	}
	if(!encontroEspacio){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		uint32_t nroBloqueDeDato = getBloqueLibre();
		agregarAInodo(inodo,nroBloqueLogico,nroBloqueDeDato);
		void * ptrInicioNuevaEntrada = posicionarme(inodo,nroBloqueLogico,0);
		agregarNuevaEntrada(ptrInicioNuevaEntrada,nameNewEntry,tamanio_bloque);
		inodo->size += tamanio_bloque;
	}
}

uint32_t tamanioMinEntradaDirectorio(char * nombre){
	uint32_t tamanio_entrada = sizeof(struct DirEntry) + strlen(nombre);
	uint32_t resto = tamanio_entrada % 4;
	uint32_t padding = (resto == 0) ? 0 : 4 - resto;
	tamanio_entrada += padding;
	return tamanio_entrada;
}

/*
 * Desc: Agrega una nueva entrada de directorio
 * Return: número de inodo al que se asigna esta nueva entrada
 */
// todo: revert si nadie usa el numero de inodo retornado
uint32_t agregarNuevaEntrada(void * ptrEntradaDirectorio,char * nombre, uint32_t sizeRestante){
	struct DirEntry * entrada_nueva = calloc(1,sizeof(struct DirEntry) + strlen(nombre) + 1);
	uint32_t nroInodoLibre = getInodoLibre();
	entrada_nueva->inode = nroInodoLibre;
	entrada_nueva->entry_len = sizeRestante;
	entrada_nueva->name_len = strlen(nombre);
	strcat(entrada_nueva->name,nombre);
	memcpy(ptrEntradaDirectorio,entrada_nueva,sizeof(struct DirEntry) + strlen(nombre));
	// todo: cargar datos correctos al inodo
	struct INode * inodo = getInodo(entrada_nueva->inode);
	inodo->mode = 16893;
	inodo->uid = 1000;
	inodo->gid = 1000;
	inodo->atime = 1340777945;
	inodo->ctime = 1340777940;
	inodo->mtime = 13407779;
	inodo->generation = 996980145;
	inodo->links = 1;
	inodo->blocks[0] = getBloqueLibre();
	inodo->size = tamanio_bloque;
	free(entrada_nueva);
	return nroInodoLibre;
}

uint32_t getInodoLibre(){
	struct Superblock *sb = read_superblock();
	if(sb->free_inodes == 0)
		perror("error: No hay inodos disponibles");
	uint32_t nroInodo = 0;
	uint32_t nro_grupo;
	for(nro_grupo = 0;nroInodo == 0;nro_grupo++)
		nroInodo = getInodoLibreDelBitmap(nro_grupo);
	actualizarEstructurasCuandoPidoInodo(nroInodo);
	return nroInodo;

}

// todo: ver donde marcar como ocupado el inodo
uint32_t getInodoLibreDelBitmap(uint32_t nro_grupo){
	uint32_t nroInodo = 0;
	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioBB = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + inicioBB * tamanio_bloque;
	struct Superblock *bloque = read_superblock();
	int cantInodos = bloque->inodes_per_group;
	int cantBytes = cantInodos / 8;
	uint32_t nroPrimerInodoDelGrupo;
	nroPrimerInodoDelGrupo =  nroInodoInicioDeGrupo(nro_grupo);
	t_bitarray 	* ptrBit = bitarray_create((char*)posActual, cantBytes);
	int i;
	for(i = 0;i < cantInodos;i++){
		bool valor = bitarray_test_bit(ptrBit, i);
		if(valor == 0){
			nroInodo = nroPrimerInodoDelGrupo + i;
			break;
		}
	}

	return nroInodo;
}


void actualizarEstructurasCuandoPidoInodo(uint32_t nroInodo){
	struct Superblock *sb = read_superblock();
	//todo : ver si funca para todos los casos(ej. con bloques de 2k, 4k)
	uint32_t nroGrupo = (nroInodo - 1) / sb->inodes_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroInodeBitmap = grupo->inode_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroInodeBitmap);
	int cantBytes = sb->inodes_per_group / 8;
	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);
	while(nroInodo > sb->inodes_per_group)
		nroInodo -= sb->inodes_per_group;
	// marca como ocupado el numero de inodo en su bitmap
	bitarray_set_bit(ptrBit,nroInodo - 1); // el -1 es porque la función empieza a leer desde cero, pero el nroBloqueDeDatos puede ser >= 1

	(sb->free_inodes)--;	// Actualizando el free_blocks del superbloque
	(grupo->free_inodes_count)--;	// Actualizando el free_blocks del group descriptor
	(grupo->used_dirs_count)++;

}

/*
 * Desc: Se crean las entradas . y .. para el nuevo directorio
 * nroInodoEntradaNueva -> es el nro de inodo que se carga para la entrada .
 * nroInodoRuta -> es el nro de inodo que se carga para la entrada ..
 */
void inicializarEntradasNuevoDir(struct INode * inodo, uint32_t offset, uint32_t nroInodoEntradaNueva, uint32_t nroInodoRuta){
	void * ptr = posicionarme(inodo,0,0);

	char * nombre = calloc(1,2);
	strncpy(nombre,".",1);

	struct DirEntry * entrada_nueva = malloc(sizeof(struct DirEntry) + strlen(nombre));
	entrada_nueva->inode = nroInodoEntradaNueva;
	uint16_t tamanio_primer_entrada = tamanioMinEntradaDirectorio(nombre);
	entrada_nueva->entry_len = tamanio_primer_entrada;
	entrada_nueva->name_len = strlen(nombre);
	strcat(entrada_nueva->name,nombre);
	memcpy(ptr,entrada_nueva,sizeof(struct DirEntry) + strlen(nombre));

	free(entrada_nueva);
	free(nombre);

	ptr = posicionarme(inodo,0,12);	// el 12 es el entry_len del directorio .

	nombre = calloc(1,3);
	strncpy(nombre,"..",2);

	entrada_nueva = malloc(sizeof(struct DirEntry) + strlen(nombre));
	entrada_nueva->inode = nroInodoRuta;
	entrada_nueva->entry_len = tamanio_bloque - tamanio_primer_entrada;
	entrada_nueva->name_len = strlen(nombre);
	strcat(entrada_nueva->name,nombre);
	memcpy(ptr,entrada_nueva,sizeof(struct DirEntry) + strlen(nombre));

	free(entrada_nueva);
	free(nombre);

}

uint32_t getNroInodoDeLaDireccionDelPath(char * path){
	char * separador = "/";
	char ** ruta_separada = string_split(path,separador);
	int tamanio_ruta_separada = size_array_before_split(path,separador);
	uint32_t nroInodoDeBusqueda = 2;	// el inodo 2 es el directorio raiz
	struct INode * inodoDeBusqueda = getInodo(nroInodoDeBusqueda);
	int i;
	for(i = 0;i < tamanio_ruta_separada;i++){
//		printf("inodo size: %u\n",inodoDeBusqueda->size);
		nroInodoDeBusqueda = buscarNroInodoEnEntradasDirectorio(inodoDeBusqueda,ruta_separada[i]);
		if(nroInodoDeBusqueda == 0){
// todo: revisar, entra aca cuando hago un crearDirectorio en el raiz
			printf("error: no existe la ruta\n");
			break;
		}
		inodoDeBusqueda = getInodo(nroInodoDeBusqueda);
	}
	return nroInodoDeBusqueda;
}

void actualizarEstructurasCuandoPidoBloque(uint32_t nroBloqueDeDato){
	// todo: encapsular comportamiento, ver con liberarbloque()
	// marcar ocupado el nroBloqueDeDato dentro de su block_bitmap
	struct Superblock *sb = read_superblock();
	//todo : ver si funca para todos los casos(ej. con bloques de 2k, 4k)
	uint32_t nroGrupo = (nroBloqueDeDato - 1) / sb->blocks_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroBlockBitmap = grupo->block_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroBlockBitmap);
	int cantBytes = sb->blocks_per_group / 8;
	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);
	while(nroBloqueDeDato > sb->blocks_per_group)
		nroBloqueDeDato -= sb->blocks_per_group;
	bitarray_set_bit(ptrBit,nroBloqueDeDato - 1); // el -1 es porque la función empieza a leer desde cero, pero el nroBloqueDeDatos puede ser >= 1
	// Fin - marcar ocupado el nroBloqueDeDato dentro de su block_bitmap

	(sb->free_blocks)--;	// Actualizando el free_blocks del superbloque
	(grupo->free_blocks_count)--;	// Actualizando el free_blocks del group descriptor
}

// todo: Algoritmo - Remover un directorio

void eliminarDirectorio(char * path){

	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	struct INode * inodoRuta = getInodo(nroInodoRuta);
	uint32_t nroInodoDirectorio;
	if(nroInodoRuta != 0) {
		nroInodoDirectorio = getNroInodoDeLaDireccionDelPath(path);
		if(nroInodoDirectorio != 0){
			if(directorioVacio(nroInodoDirectorio)){
				eliminarEntradaDirectorio(inodoRuta,ruta_separada->nombre);
				liberarInodo(nroInodoDirectorio);
			} else {
				printf("error: el directorio %s contiene archivos",ruta_separada->nombre);
			}
		} else {
			printf("error: no existe el directorio %s",ruta_separada->nombre);
		}
	}
	else
		printf("error: no existe la ruta");
	free(ruta_separada->ruta);
	free(ruta_separada->nombre);
	free(ruta_separada);

//	if(existeLaRuta?(path))
//		if(estaVacioDirectorio(path->nombre_entrada))
//			remover(inodo_ruta,nombre_entrada);
//		else
//			printf("El directorio no está vacío, no se puede eliminar");
//	else
//		printf("no existe la ruta o el nombre de la entrada");
}

void eliminarEntradaDirectorio(struct INode * inodoRuta, char * nombre_entrada){
	uint32_t tamanio_directorio = inodoRuta->size;
	uint32_t offset;
	int directorioEliminado = 0;
	uint32_t tamanio_dir_eliminado;
	uint32_t nroBloqueLogico;
	uint32_t tamanioEntradaAnterior;
	void * ptr;
	for(offset = 0;!directorioEliminado && offset < tamanio_directorio;offset += tamanio_bloque){
		tamanioEntradaAnterior = 0;
		nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		ptr = posicionarme(inodoRuta,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanio_bloque > cantidad){
			// todo: ver si carga bien los datos a la estructura
			struct DirEntry * directorio = (struct DirEntry *) ptr;
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			if(string_equals_ignore_case(nombre,nombre_entrada)){
				tamanio_dir_eliminado = directorio->entry_len;
				directorioEliminado = 1;
				free(nombre);
				break;
			}
			free(nombre);
			tamanioEntradaAnterior = directorio->entry_len;
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}
	}
	// es el caso cuando una sola entrada de directorio se encuentra en un bloque
	if(tamanio_dir_eliminado == tamanio_bloque){
		uint32_t * ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoRuta,nroBloqueLogico);
		liberarBloque(ptrBloqueLogico);
		ptrBloqueLogico = 0;
	} else {
		ptr -= tamanioEntradaAnterior;
		struct DirEntry * directorio = (struct DirEntry *) ptr;
		directorio->entry_len += tamanio_dir_eliminado;
	}
}

void liberarInodo(uint32_t nroInodoDirectorio){
	struct INode * inodoDirEliminado = getInodo(nroInodoDirectorio);
	uint32_t tamanio = inodoDirEliminado->size;
	uint32_t offset;
	uint32_t nroBloqueLogico;
	uint32_t * ptrBloqueLogico;
// se limpia el inodo
	for(offset = 0;offset < tamanio;offset+=tamanio_bloque){
		nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoDirEliminado,nroBloqueLogico);
		liberarBloque(ptrBloqueLogico);
	}
	// en agregarNuevaEntrada se setearon estos campos todo: encapsular en función
	inodoDirEliminado->mode = 0;
	inodoDirEliminado->uid = 0;
	inodoDirEliminado->gid = 0;
	inodoDirEliminado->atime = 0;
	inodoDirEliminado->ctime = 0;
	inodoDirEliminado->mtime = 0;
	inodoDirEliminado->generation = 0;
	inodoDirEliminado->links = 0;
	inodoDirEliminado->size = 0;

	struct Superblock *sb = read_superblock();
	//todo : ver si funca para todos los casos
	uint32_t nroGrupo = (nroInodoDirectorio - 1) / sb->inodes_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroInodeBitmap = grupo->inode_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroInodeBitmap);
	int cantBytes = sb->inodes_per_group / 8;
	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);
	while(nroInodoDirectorio > sb->inodes_per_group)
		nroInodoDirectorio -= sb->inodes_per_group;
	//marco como libre el inodo en el bitmap de bloques
	bitarray_clean_bit(ptrBit,nroInodoDirectorio - 1); // el -1 es porque la función empieza a leer desde cero, pero el nroDeInodos puede ser >= 1
	(grupo->free_inodes_count)++;	//aumento la cantidad de inodos libres en el group descriptor
	(sb->free_inodes)++;			//aumento la cantidad de inodos libres en el superbloque
	(grupo->used_dirs_count)--;
}

int directorioVacio(uint32_t nroInodoDirectorio){
	struct INode * inodoDirectorio = getInodo(nroInodoDirectorio);
	int estaVacio = 1;
	if(inodoDirectorio->size > tamanio_bloque){
		estaVacio = 0;
	} else {
		void * ptr;
		// me posiciono directamente en la entrada de directorio ..
		uint32_t tamanio_primer_entrada = 12;
		ptr = posicionarme(inodoDirectorio,0,12);
		struct DirEntry * directorio = (struct DirEntry *) ptr;
		if(directorio->entry_len != tamanio_bloque - tamanio_primer_entrada)
			estaVacio = 0;
	}
	return estaVacio;
}

// Fin - Remover un directorio

// Algoritmo - Crear un archivo

// todo: comprobar que ruta_separada->ruta es directorio
void crearArchivo(char * path, uint32_t mode){
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	struct INode * inodoRuta = getInodo(nroInodoRuta);
	if(nroInodoRuta != 0)
		if(getNroInodoDeLaDireccionDelPath(path) == 0){
			agregarEntradaDirectorio(inodoRuta,ruta_separada->nombre);
			uint32_t nroInodoEntradaNueva = getNroInodoDeLaDireccionDelPath(path);
			struct INode * inodoDeArchivo = getInodo(nroInodoEntradaNueva);
			setearInodo(inodoDeArchivo,mode);
			free(ruta_separada->ruta);
			free(ruta_separada->nombre);
			free(ruta_separada);
		} else {
			printf("error: ya existe el archivo %s",ruta_separada->nombre);
		}
	else
		printf("error: no existe la ruta");

}

void setearInodo(struct INode * inodoArchivo, uint32_t mode){
	inodoArchivo->mode = mode;
	inodoArchivo->uid = 1000;
	inodoArchivo->gid = 1000;
	inodoArchivo->atime = 1340777945;
	inodoArchivo->ctime = 1340777940;
	inodoArchivo->mtime = 13407779;
	inodoArchivo->generation = 322993457;
	inodoArchivo->links = 1;
	inodoArchivo->size = 0;
}

// Fin - Crear un archivo
