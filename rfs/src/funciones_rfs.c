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

t_list * listarDirectorio(char * path){
	struct INode * inodoDeBusqueda = getInodoDeLaDireccionDelPath(path);
	// todo: cargar en las estructuras
	return cargarEntradasDirectorioALista(inodoDeBusqueda);
}

struct INode * buscarInodoEnEntradasDirectorio(struct INode * inodoDeBusqueda,char * ruta){

	struct INode * inodoBuscado = NULL;
	uint32_t tamanio_directorio = inodoDeBusqueda->size;
	uint32_t offset;
	for(offset = 0;inodoBuscado == NULL && offset < tamanio_directorio;offset += tamanio_bloque){
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
				inodoBuscado = getInodo(directorio->inode);
				break;
			}
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}

	}

	return inodoBuscado;
}

t_list * cargarEntradasDirectorioALista(struct INode * inodo){
	t_list * lista = list_create();

	struct INode * inodoEntradaDirectorioActual;
	uint32_t tamanio_directorio = inodo->size;
	uint32_t offset;
	for(offset = 0;offset < tamanio_directorio;offset += tamanio_bloque){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodo,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanio_bloque > cantidad){
			// todo: ver si carga bien los datos a la estructura
			struct DirEntry * directorio = (struct DirEntry *) ptr;
			printf("nombre %s\n",directorio->name);
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			inodoEntradaDirectorioActual = getInodo(directorio->inode);
			t_readdir_stat * readdirStats = malloc(sizeof(t_readdir_stat));
			readdirStats->mode = inodoEntradaDirectorioActual->mode;
			readdirStats->gid = inodoEntradaDirectorioActual->gid;
			readdirStats->nlink = inodoEntradaDirectorioActual->links;
			readdirStats->uid = inodoEntradaDirectorioActual->uid;
			readdirStats->name = nombre;
			list_add(lista,inodoEntradaDirectorioActual);

			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
		}

	}
	return lista;
}

void leerArchivo(char * path,uint32_t offset,uint32_t size){

	struct INode * inodoDeBusqueda = getInodoDeLaDireccionDelPath(path);
	if((size <= 0) || (inodoDeBusqueda->size <= offset + size))
		perror("no es posible realizar la lectura");
	guardarDatosArchivos(inodoDeBusqueda,offset,size);
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
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t *);
	return nroBloqueLogico < 12 + cantBloquesPorIndireccion;
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
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t *);
	return nroBloqueLogico < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2);
}

uint32_t * posicionarIndireccionDoble(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanio_bloque / sizeof(uint32_t *);
	int i;
	uint32_t cantBloquesYaRecorridos = 267;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	for (i = 0; cantBloquesPorIndireccionSimple <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionSimple);
	ptrBloque = (uint32_t*) (inicio + i * 4);	// me ubico en la indireccion doble que contiene el bloque logico que busco
	inicio = posicionarInicioBloque(*ptrBloque);
	ptrBloque = (uint32_t *) (inicio + nroBloqueLogico * 4); // dentro de la indireccion doble, me ubico en la indireccion simple
	return ptrBloque;
}

int esIndireccionTriple(uint32_t nroBloque){
	uint32_t cantBloquesPorIndireccion = tamanio_bloque / sizeof(uint32_t *);
	return nroBloque < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2)
			+ pow(cantBloquesPorIndireccion,3);
}

uint32_t * posicionarIndireccionTriple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanio_bloque / sizeof(uint32_t *);
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
	struct INode * inodoDeBusqueda = getInodo(2); // el inodo 2 es el directorio raíz
	int i;
	for(i = 0;i < tamanio_ruta_separada;i++){
		printf("inodo size: %u\n",inodoDeBusqueda->size);
		inodoDeBusqueda = buscarInodoEnEntradasDirectorio(inodoDeBusqueda,ruta_separada[i]);
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
			// todo: controlar si hay suficientes espacio
			while(size > 0){
				uint32_t resto = tamanio_bloque - desplazamientoDentroDelBloque(offsetEOF);
				if(resto == tamanio_bloque){
					uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offsetEOF);
					uint32_t nroBloqueDeDato = getBloqueLibre();
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
	// le asigno el número de bloque de dato nuevo, al número de bloque lógico dentro del inodo
	uint32_t * ptrBloque = getPtrNroBloqueLogicoDentroInodo(inodoPath,nroBloqueLogico);

	// Le asigno al número de bloque relativo del inodo, el número de bloque de dato libre
	uint32_t * ptrNroBloqueDeDato = &nroBloqueDeDato;
	memcpy(ptrBloque,ptrNroBloqueDeDato,sizeof(uint32_t));

	(sb->free_blocks)--;	// Actualizando el free_blocks del superbloque
	(grupo->free_blocks_count)--;	// Actualizando el free_blocks del group descriptor
}

void completarBloque(struct INode * inodo, uint32_t offset, uint32_t size){
	uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
	uint32_t desplazamiento = desplazamientoDentroDelBloque(offset);
	void * ptr = posicionarme(inodo,nroBloqueLogico,desplazamiento);
	escribirBloque(ptr,size);
}

void escribirBloque(void * posicionPtr,uint32_t size){
	uint32_t i;
	char * dato = "C";
	for(i = 0;i < size;i++,posicionPtr++)
		memcpy(posicionPtr,dato,1);
}

void escribirArchivo(char * path, char * input, uint32_t size, uint32_t offset){

	// todo: hacer un control si hay espacio en el disco
	struct INode * inodoPath = getInodoDeLaDireccionDelPath(path);
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
