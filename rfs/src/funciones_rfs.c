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
#include "src/nipc/nipc.h"
#include "../commons/src/commons/log.h"
#include <errno.h>
#include <pthread.h>
#include "administracion.h"
#include <libmemcached/memcached.h>
#include "../commons/src/commons/misc.h"
#include "../commons/src/commons/string.h"

extern t_list * archivos_abiertos;

static uint32_t nro_de_grupo = 0;
static pthread_mutex_t sem_escribir;
static pthread_mutex_t sem_liberar_bloque;
static pthread_mutex_t sem_pedir_bloque;
static pthread_mutex_t sem_crear_dir;
static pthread_mutex_t sem_inodo_new_dir;
static pthread_mutex_t sem_separar_path;
static pthread_mutex_t sem_liberar_inodo;
static pthread_mutex_t sem_crear_archivo;
static pthread_mutex_t sem_eliminar_archivo;
static pthread_mutex_t sem_truncar_archivo;

static int tamanio_bloque = 0;
uint8_t *ptr_arch;
t_log *logger_funciones;

void set_logger_funciones(t_log *logger_para_funciones) {
    logger_funciones = logger_para_funciones;
}

void init_semaforos(void) {
	pthread_mutex_init(&sem_escribir, NULL);
	pthread_mutex_init(&sem_liberar_bloque, NULL);
	pthread_mutex_init(&sem_pedir_bloque, NULL);
	pthread_mutex_init(&sem_crear_dir, NULL);
	pthread_mutex_init(&sem_inodo_new_dir, NULL);
	pthread_mutex_init(&sem_separar_path, NULL);
	pthread_mutex_init(&sem_liberar_inodo, NULL);
	pthread_mutex_init(&sem_crear_archivo, NULL);
	pthread_mutex_init(&sem_eliminar_archivo, NULL);
	pthread_mutex_init(&sem_truncar_archivo, NULL);
}

void destroy_semaforos(void) {
	pthread_mutex_destroy(&sem_escribir);
	pthread_mutex_destroy(&sem_liberar_bloque);
	pthread_mutex_destroy(&sem_pedir_bloque);
	pthread_mutex_destroy(&sem_crear_dir);
	pthread_mutex_destroy(&sem_inodo_new_dir);
	pthread_mutex_destroy(&sem_separar_path);
	pthread_mutex_destroy(&sem_liberar_inodo);
	pthread_mutex_destroy(&sem_crear_archivo);
	pthread_mutex_destroy(&sem_eliminar_archivo);
	pthread_mutex_destroy(&sem_truncar_archivo);
}

int tamanioDeBloque() {
    if(tamanio_bloque == 0){
        tamanio_bloque = 1024 << read_superblock()->log_block_size;
    }
    return tamanio_bloque;
}

struct DirEntry *leerEntrada(void *disco) {
    struct DirEntry *instancia = malloc(sizeof(struct DirEntry));
    size_t offset = 0;
    memcpy(&instancia->inode, disco, sizeof(instancia->inode));
    offset += sizeof(instancia->inode);
    memcpy(&instancia->entry_len, disco + offset, sizeof(instancia->entry_len));
    offset += sizeof(instancia->entry_len);
    memcpy(&instancia->name_len, disco + offset, sizeof(instancia->name_len));
    offset += sizeof(instancia->name_len);
    memcpy(&instancia->type, disco + offset, sizeof(instancia->type));
    offset += sizeof(instancia->type);
    instancia->name = malloc(instancia->name_len);
    memcpy(instancia->name, disco + offset, instancia->name_len);

    return instancia;
}

void grabarEntrada(void *disco, struct DirEntry *entrada) {
    size_t offset = 0;
    memcpy(disco, &entrada->inode, sizeof(entrada->inode));
    offset += sizeof(entrada->inode);
    memcpy(disco + offset, &entrada->entry_len, sizeof(entrada->entry_len));
    offset += sizeof(entrada->entry_len);
    memcpy(disco + offset, &entrada->name_len, sizeof(entrada->name_len));
    offset += sizeof(entrada->name_len);
    memcpy(disco + offset, &entrada->type, sizeof(entrada->type));
    offset += sizeof(entrada->type);
    memcpy(disco + offset, entrada->name, entrada->name_len);
    free(entrada->name);
    free(entrada);
}

void mapear_archivo(char *ruta_archivo) {

	uint32_t archivo = open(ruta_archivo, O_RDWR);
	if (archivo == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	struct stat fd;
	fstat(archivo, &fd);

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

struct Superblock * read_superblock() {

	uint16_t OFFSET = 1024;
	struct Superblock *sb;
	uint8_t *posicion = ptr_arch + OFFSET;
	sb = (struct Superblock *) posicion;
	return sb;

}

uint32_t cantidadDeGrupos() {
	struct Superblock *sb= read_superblock();
	uint32_t inodos = sb->inodes;
	uint32_t inodosPorGrupo = sb->inodes_per_group;
	double resto = inodos % inodosPorGrupo;
	int cantidadDeGrupos = inodos / inodosPorGrupo;
	return (resto != 0) ? cantidadDeGrupos + 1 : cantidadDeGrupos;
}

struct GroupDesc * leerGroupDescriptor(uint32_t nroGrupo){
	struct GroupDesc * gd = NULL;
	if(nroGrupo < cantidadDeGrupos()){
		uint16_t posInicialGD = (tamanioDeBloque() == 4096) ? tamanioDeBloque() : 2048;
		uint8_t tamanioGrupo = sizeof(struct GroupDesc);
		uint8_t * posActual = ptr_arch + posInicialGD + (tamanioGrupo * nroGrupo);
		gd = (struct GroupDesc*) posActual;
	}
	return gd;
}

uint32_t nroBloqueInicioDeGrupo(uint32_t nro_grupo){
	struct Superblock *sb = read_superblock();
	uint32_t bloquesPorGrupo = sb->blocks_per_group;
	uint32_t nroBloqueInicio;
	nroBloqueInicio = sb->first_data_block;
	return nroBloqueInicio + bloquesPorGrupo * nro_grupo;
}

uint32_t nroInodoInicioDeGrupo(uint32_t nro_grupo){
	struct Superblock *sb = read_superblock();
	uint32_t inodosPorGrupo = sb->inodes_per_group;
	return inodosPorGrupo * nro_grupo + 1; //el + 1 es porque empieza en el inodo nro 1
}

struct INode * getInodo(uint32_t nroInodo){
    if(nroInodo < 1) {
        return NULL;
    }

	log_trace(logger_funciones, "nroInodo: %hu\n",nroInodo);
	struct Superblock *sb = read_superblock();
	int nroGrupo = (nroInodo - 1) / sb->inodes_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroBloqueTablaInodos = grupo->inode_table;
	uint8_t *dir_tabla_inodos = ptr_arch + nroBloqueTablaInodos * tamanioDeBloque();
	uint8_t tamanioInodo = sizeof(struct INode);

	int cantInodos = nroInodo - nroGrupo * sb->inodes_per_group;

	struct INode* inodo;
	int i;
	for(i=1;i<cantInodos;i++,dir_tabla_inodos += tamanioInodo);
	inodo = (struct INode*) dir_tabla_inodos;
	return inodo;

}

void *obtenerBloque(uint32_t numero_bloque) {
    return ptr_arch + numero_bloque * tamanioDeBloque();
}

uint8_t * posicionarInicioBloque(uint32_t nroBloque){
	return ptr_arch + nroBloque * tamanioDeBloque();
}

// de prueba todo: controlar que el path es un directorio
t_list * listarDirectorio(char * path){
	uint32_t nroInodoDeRuta;
	uint32_t nroInodoDeDirectorio;
	struct INode * inodo;
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	nroInodoDeRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
    free(ruta_separada->nombre);
	free(ruta_separada->ruta);
	free(ruta_separada);
	if(nroInodoDeRuta != 0){
		nroInodoDeDirectorio = getNroInodoDeLaDireccionDelPath(path);
		if(nroInodoDeDirectorio != 0){
			inodo = getInodo(nroInodoDeDirectorio);
			return cargarEntradasDirectorioALista(inodo);
		} else {
			printf("no existe el archivo");
			return list_create();
		}
	} else {
		printf("no existe la ruta");
		return list_create();
	}
}

uint32_t buscarNroInodoEnEntradasDirectorio(struct INode * inodoDeBusqueda,char * ruta){

	uint32_t nroInodoBuscado = 0;
	uint32_t tamanio_directorio = inodoDeBusqueda->size;
	uint32_t offset;
	for(offset = 0;nroInodoBuscado == 0 && offset < tamanio_directorio;offset += tamanioDeBloque()){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodoDeBusqueda,nroBloqueLogico,0);
        struct DirEntry * directorio = leerEntrada(ptr);

		uint16_t cantidad = 0;
		int i = 0;
		while(tamanioDeBloque() > cantidad && directorio->entry_len){
			log_trace(logger_funciones, "entrada de directorio %d\n",i++);
			log_trace(logger_funciones, "nro inodo: %u\n",directorio->inode);
			log_trace(logger_funciones, "entry_len: %u\n",directorio->entry_len);
			log_trace(logger_funciones, "name_len: %u\n",directorio->name_len);
			log_trace(logger_funciones, "type: %u\n",directorio->type);
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			log_trace(logger_funciones, "name: %s\n", nombre);
			if(string_equals_ignore_case(nombre,ruta)){
				log_trace(logger_funciones, "nro de inodo que gestiona %s es %u\n",ruta,directorio->inode);
				nroInodoBuscado = directorio->inode;
				free(directorio->name);
                free(directorio);
				free(nombre);
				break;
			}
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
			free(nombre);
			free(directorio->name);
			free(directorio);
			directorio = leerEntrada(ptr);
		}

	}

	return nroInodoBuscado;
}

t_list * cargarEntradasDirectorioALista(struct INode * directorio){
    t_list *entradas = list_create();

	uint32_t tamanio_directorio = directorio->size;
	uint32_t offset;
	for(offset = 0;offset < tamanio_directorio;offset += tamanioDeBloque()){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(directorio,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanioDeBloque() > cantidad){
			struct DirEntry * entradaDirectorio = leerEntrada(ptr);
			struct INode * inodoEntradaDirectorio = getInodo(entradaDirectorio->inode);
			if(inodoEntradaDirectorio == NULL) {
			    free(entradaDirectorio->name);
                free(entradaDirectorio);
			    break;
			}

			struct readdir_entry *entrada = malloc(sizeof(struct readdir_entry));

			entrada->path = calloc(1, entradaDirectorio->name_len + 1);
			memcpy(entrada->path, entradaDirectorio->name, entradaDirectorio->name_len);

			/*
			 * TODO: si el inodo tiene todo en 0 (x ej, mode, n_link, size) la implementacion
			 * de ext2 del kernel pareciera ignorar esa entrada (supongo que un n_link = 0
			 * esta mal mal mal)
			 */
			entrada->mode = inodoEntradaDirectorio->mode;
			entrada->n_link = inodoEntradaDirectorio->links;
			entrada->size = inodoEntradaDirectorio->size;
			entrada->blocks = inodoEntradaDirectorio->nr_blocks;

			list_add(entradas,entrada);

			cantidad += entradaDirectorio->entry_len;
			ptr += entradaDirectorio->entry_len;

			free(entradaDirectorio->name);
			free(entradaDirectorio);
		}
	}
	return entradas;
}

size_t leerArchivo(char * path, uint32_t offset, uint32_t bytesALeer, void **bufferPointer) {
    *bufferPointer = NULL;
    uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(path);
    struct archivo_abierto * registro_archivo = getRegistroArchivoAbierto(numero_inodo);

    if(registro_archivo != NULL) {
        size_t bytesLeidos = 0;
        pthread_rwlock_rdlock(registro_archivo->lock);
        struct INode * inodoDeBusqueda = getInodo(numero_inodo);

        // Si el archivo esta vacio, no leo
        if(inodoDeBusqueda->size != 0) {

            // Si me piden leer mas que el largo del archivo, achico la cantidad a leer
            if(inodoDeBusqueda->size <= offset + bytesALeer) {
                bytesALeer = inodoDeBusqueda->size - offset;
            }

            void *buffer = malloc(bytesALeer);

            while(bytesLeidos < bytesALeer) {
                uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset + bytesLeidos);
                uint32_t desplazamiento = desplazamientoDentroDelBloque(offset + bytesLeidos);
                void *ptr = posicionarme(inodoDeBusqueda, nroBloqueLogico, desplazamiento);


                if(bytesALeer - bytesLeidos > tamanioDeBloque()){
                    // El contenido sigue en otro bloque. Leo el resto de este bloque y sigo con los otros
                    uint32_t restoDelBloque = tamanioDeBloque() - desplazamiento;
                    memcpy(buffer + bytesLeidos, ptr, restoDelBloque);
                    bytesLeidos += restoDelBloque;
                } else {
                    // El contenido termina en este bloque. Leo lo que falta de contenido
                    memcpy(buffer + bytesLeidos, ptr, bytesALeer - bytesLeidos);
                    bytesLeidos = bytesALeer;
                }
            }

            *bufferPointer = buffer;
        }

        pthread_rwlock_unlock(registro_archivo->lock);
        return bytesLeidos;
    } else {
        return ENOENT;
    }
}

uint32_t nroBloqueDentroDelInodo(uint32_t offset){
	return offset / tamanioDeBloque();
}

uint32_t desplazamientoDentroDelBloque(uint32_t offset){
	return offset % tamanioDeBloque();
}

int esBloqueDirecto(uint32_t nroBloque) {
	return nroBloque < 12;
}

int esIndireccionSimple(uint32_t nroBloqueLogico) {
	uint32_t cantBloquesPorIndireccion = tamanioDeBloque() / sizeof(uint32_t);
	return (12 <= nroBloqueLogico) && (nroBloqueLogico < 12 + cantBloquesPorIndireccion);
}

uint32_t * posicionarIndireccionSimple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico) {
	uint8_t* inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t* ptrBloque;
	int i;
	uint32_t cantBloquesYaRecorridos = 12;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	for (i = 0; i < nroBloqueLogico; i++);
	ptrBloque = (uint32_t*) (inicio + i * 4);
//	log_trace(logger_funciones, "nro de bloque: %u\n", *ptrBloque);
	return ptrBloque;
}

int esIndireccionDoble(uint32_t nroBloqueLogico){
	uint32_t cantBloquesPorIndireccion = tamanioDeBloque() / sizeof(uint32_t);
	return (12 + cantBloquesPorIndireccion <= nroBloqueLogico) && (nroBloqueLogico < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2));
}

uint32_t * posicionarIndireccionDoble(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
	int i;
	uint32_t cantBloquesYaRecorridos = 12 + cantBloquesPorIndireccionSimple;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	for (i = 0; cantBloquesPorIndireccionSimple <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionSimple);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	inicio = posicionarInicioBloque(*ptrBloque);
	ptrBloque = (uint32_t *) (inicio + nroBloqueLogico * 4);
	return ptrBloque;
}

int esIndireccionTriple(uint32_t nroBloque){
	uint32_t cantBloquesPorIndireccion = tamanioDeBloque() / sizeof(uint32_t);
	return (12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2) <= nroBloque) && (nroBloque < 12 + cantBloquesPorIndireccion + pow(cantBloquesPorIndireccion,2)
			+ pow(cantBloquesPorIndireccion,3));
}

uint32_t * posicionarIndireccionTriple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
	uint32_t cantBloquesPorIndireccionDoble = pow(cantBloquesPorIndireccionSimple,2);
	uint32_t cantBloquesYaRecorridos = 12 + cantBloquesPorIndireccionSimple + cantBloquesPorIndireccionDoble;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	int i,j;
	for (i = 0; cantBloquesPorIndireccionDoble <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionDoble);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	inicio = posicionarInicioBloque(*ptrBloque);
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
	log_trace(logger_funciones, "nroBloqueDeDato %u\n",*ptrNroBloqueLogico);
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
    uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(path);
    if(numero_inodo == 0) {
        return NULL;
    } else {
        return getInodo(numero_inodo);
    }
}

int32_t truncarArchivo(char * path,uint32_t offset){

	pthread_mutex_lock(&sem_truncar_archivo);
	int32_t resultado = 0;
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	if(nroInodoRuta != 0)
		if(getNroInodoDeLaDireccionDelPath(path) != 0){

			if(offset < 0){
				perror("error: el offset debe ser >= 0");
			} else {
				struct INode * inodoPath = getInodoDeLaDireccionDelPath(path);
				uint32_t offsetEOF = inodoPath->size; // el -1 es para sacarle el EOF cuando trabajo con un archivo creado externamente
				int32_t size = offset - offsetEOF;
				uint32_t cantPtrEnIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
				uint32_t cantPtrEnIndireccionDoble = cantPtrEnIndireccionSimple * cantPtrEnIndireccionSimple;
				if(size == 0)
					log_trace(logger_funciones, "no hay cambio");
				if(size < 0){
					if(offsetEOF < abs(size)){ // si el size es negativo y es mayor en valor absoluto que el tamanio del archivo: error
						resultado = EINVAL;
						log_error(logger_funciones, "truncamiento invalido: %s", strerror(EINVAL));
				}else{
						// achicar archivo
						while(size < 0){
							uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offsetEOF - 1);	//el -1 es por el EOF
							uint32_t desplazamiento = desplazamientoDentroDelBloque(offsetEOF - 1) + 1;	//para que cuando me den un offset multiplo de tamanio_bloque, el desplazamiento me devuelva tamanio_bloque
							if(desplazamiento <= abs(size)){
								uint32_t * ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoPath,nroBloqueLogico);
								liberarBloque(ptrBloqueLogico);
								inodoPath->nr_blocks -= tamanioDeBloque() / 512;
								offsetEOF -= desplazamiento;
								size += desplazamiento;

								if(esIndireccionSimple(nroBloqueLogico) && nroBloqueLogico == 12){
									liberarBloque(&inodoPath->iblock);
									inodoPath->nr_blocks -= tamanioDeBloque() / 512;
								}
								if(esIndireccionDoble(nroBloqueLogico)){
									// el 12 es por los bloques que ya recorrio
									if((nroBloqueLogico - 12) % cantPtrEnIndireccionSimple == 0){
										uint32_t posDentroIndireccionDoble = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple)) / cantPtrEnIndireccionSimple;
										void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
										uint32_t * nroBloque = (uint32_t *)ptrBloque;
										liberarBloque(nroBloque);
										inodoPath->nr_blocks -= tamanioDeBloque() / 512;
									}
									if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple){
										liberarBloque(&inodoPath->iiblock);
										inodoPath->nr_blocks -= tamanioDeBloque() / 512;
									}
								}
								if(esIndireccionTriple(nroBloqueLogico)){
									void * ptrIndDoble = getPtrBloqueIndDoble(inodoPath->iiiblock,nroBloqueLogico);
									uint32_t * nroBloqueIndDoble = (uint32_t *)ptrIndDoble;
									void * ptrIndTriple = getPtrBloqueIndTriple(inodoPath->iiiblock,nroBloqueLogico);
									uint32_t * nroBloqueIndTriple = (uint32_t *)ptrIndTriple;
									if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionSimple == 0){
										liberarBloque(nroBloqueIndDoble);
										inodoPath->nr_blocks -= tamanioDeBloque() / 512;
									}
									if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionDoble == 0){
										liberarBloque(nroBloqueIndTriple);
										inodoPath->nr_blocks -= tamanioDeBloque() / 512;
									}
									if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble){
										liberarBloque(&inodoPath->iiiblock);
										inodoPath->nr_blocks -= tamanioDeBloque() / 512;
									}
								}
							} else {
								offsetEOF += size;
								size -= size;
							}
						}
						inodoPath->size = offsetEOF;
					}
				} else {
					//agrandar archivo
					if(hayEspacioSuficiente(size)){
						while(size > 0){
							uint32_t resto = tamanioDeBloque() - desplazamientoDentroDelBloque(offsetEOF);
							if(resto == tamanioDeBloque()){
								uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offsetEOF);
								uint32_t nroBloqueDeDato = getBloqueLibre();
								inodoPath->nr_blocks += tamanioDeBloque() / 512;
			// manejo de la asignación de bloques para indirecciones simples, dobles y triples
								if(esIndireccionSimple(nroBloqueLogico) && nroBloqueLogico == 12)
										inodoPath->iblock = getBloqueLibre();
								if(esIndireccionDoble(nroBloqueLogico)){
									if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple){
										inodoPath->iiblock = getBloqueLibre();
										inodoPath->nr_blocks += tamanioDeBloque() / 512;
									}
									// el 12 es por los bloques que ya recorrio
									if((nroBloqueLogico - 12) % cantPtrEnIndireccionSimple == 0){
										uint32_t posDentroIndireccionDoble = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple)) / cantPtrEnIndireccionSimple;
										void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
										uint32_t nroBloqueLibre = getBloqueLibre();
										inodoPath->nr_blocks += tamanioDeBloque() / 512;
										uint32_t * ptrNroBloqueLibre = &nroBloqueLibre;
										memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
									}
								}
								if(esIndireccionTriple(nroBloqueLogico)){
									if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble){
										inodoPath->iiiblock = getBloqueLibre();
										inodoPath->nr_blocks += tamanioDeBloque() / 512;
									}
									uint32_t nroBloqueLibre;
									if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionDoble == 0){
										uint32_t posDentroIndireccionTriple = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) / cantPtrEnIndireccionDoble;
										void * ptrBloque = desplazarme(inodoPath->iiiblock,posDentroIndireccionTriple * 4);
										nroBloqueLibre = getBloqueLibre();
										inodoPath->nr_blocks += tamanioDeBloque() / 512;
										uint32_t * ptrNroBloqueLibre = &nroBloqueLibre;
										memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
									}
									if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionSimple == 0){
										void * ptrIndTriple = getPtrBloqueIndTriple(inodoPath->iiiblock,nroBloqueLogico);
										uint32_t * nroBloqueIndTriple = (uint32_t *)ptrIndTriple;
										uint32_t nroBloqueRelativo = nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble);
										while(nroBloqueRelativo >= cantPtrEnIndireccionDoble)
											nroBloqueRelativo -= cantPtrEnIndireccionDoble;
										uint32_t posDentroIndireccionDoble = nroBloqueRelativo / cantPtrEnIndireccionSimple;
										void * ptrBloque = desplazarme(*nroBloqueIndTriple,posDentroIndireccionDoble * 4);
										uint32_t nroBloqueLibreIndDoble = getBloqueLibre();
										inodoPath->nr_blocks += tamanioDeBloque() / 512;
										uint32_t * ptrNroBloqueLibre = &nroBloqueLibreIndDoble;
										memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
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
					} else{
						resultado = ENOSPC;
						log_error(logger_funciones, "no hay suficiente espacion en disco");
					}
					}
			}

		} else {
			resultado = ENOENT;
			log_error(logger_funciones, "no existe el archivo %s",ruta_separada->nombre);
		}
	else{
		resultado = ENOENT;
		log_error(logger_funciones, "error: no existe la ruta");
	}
	free(ruta_separada->ruta);
	free(ruta_separada->nombre);
	free(ruta_separada);

	pthread_mutex_unlock(&sem_truncar_archivo);
	return resultado;
}


void * getPtrBloqueIndDoble(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
	uint32_t cantBloquesPorIndireccionDoble = pow(cantBloquesPorIndireccionSimple,2);
	uint32_t cantBloquesYaRecorridos = 12 + cantBloquesPorIndireccionSimple + cantBloquesPorIndireccionDoble;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	int i,j;
	for (i = 0; cantBloquesPorIndireccionDoble <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionDoble);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	inicio = posicionarInicioBloque(*ptrBloque);
	for (j = 0; cantBloquesPorIndireccionSimple <= nroBloqueLogico; j++, nroBloqueLogico -= cantBloquesPorIndireccionSimple);
	ptrBloque = (uint32_t*) (inicio + j * 4);
	return ptrBloque;
}

void * getPtrBloqueIndTriple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico){
	uint8_t * inicio = posicionarInicioBloque(nroBloqueIndireccion);
	uint32_t * ptrBloque;
	uint32_t cantBloquesPorIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
	uint32_t cantBloquesPorIndireccionDoble = pow(cantBloquesPorIndireccionSimple,2);
	uint32_t cantBloquesYaRecorridos = 12 + cantBloquesPorIndireccionSimple + cantBloquesPorIndireccionDoble;
	nroBloqueLogico -= cantBloquesYaRecorridos;
	int i;
	for (i = 0; cantBloquesPorIndireccionDoble <= nroBloqueLogico; i++, nroBloqueLogico -= cantBloquesPorIndireccionDoble);
	ptrBloque = (uint32_t*) (inicio + i * 4);
	return ptrBloque;
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
	pthread_mutex_lock(&sem_liberar_bloque);
	uint32_t nroBloque = 0;
	uint32_t * ptrNroBloque = &nroBloque;
	uint32_t nroBloqueDeDato = *ptrBloqueDeDato;
	struct Superblock *sb = read_superblock();
	uint32_t nroGrupo = (nroBloqueDeDato - sb->first_data_block) / sb->blocks_per_group;
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
	free(ptrBit);
	pthread_mutex_unlock(&sem_liberar_bloque);
}

uint32_t getBloqueLibre(){

	pthread_mutex_lock(&sem_pedir_bloque);
	uint32_t nroBloqueDeDato = 0;
	struct GroupDesc * gd = leerGroupDescriptor(nro_de_grupo);
	char diLaVuelta = 0;
	while(gd == NULL || gd->free_blocks_count == 0){
        nro_de_grupo++;
		if(nro_de_grupo == cantidadDeGrupos()) {
		    if(diLaVuelta) {
		        log_error(logger_funciones, "No hay bloques libres");
		        pthread_mutex_unlock(&sem_pedir_bloque);
		        return 0;
		    } else {
                nro_de_grupo = 0;
                diLaVuelta = 1;
		    }
		}
		gd = leerGroupDescriptor(nro_de_grupo);
	}
    nroBloqueDeDato = getBloqueLibreDelBitmap(nro_de_grupo);
	actualizarEstructurasCuandoPidoBloque(nroBloqueDeDato);
	pthread_mutex_unlock(&sem_pedir_bloque);
	return nroBloqueDeDato;
}

uint32_t getBloqueLibreDelBitmap(uint32_t nro_grupo){
	uint32_t nroBloqueDeDato = 0;
	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioBB = grupo->block_bitmap;
	uint8_t *posActual = ptr_arch + inicioBB * tamanioDeBloque();
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
	free(ptrBit);
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

int32_t escribirArchivo(char * path, void * input, uint32_t size, uint32_t offset){

	int32_t resultado = 0;
	pthread_mutex_lock(&sem_escribir);
	struct archivo_abierto * registro_archivo = getRegistroArchivoAbierto(getNroInodoDeLaDireccionDelPath(path));
	pthread_mutex_unlock(&sem_escribir);

	if(registro_archivo != NULL && registro_archivo->cantidad_abiertos > 0){
		pthread_rwlock_wrlock(registro_archivo->lock);
		uint32_t nroInodoPath = getNroInodoDeLaDireccionDelPath(path);
		if(nroInodoPath == 0){
			resultado = ENOENT;
			log_error(logger_funciones, "no existe el archivo");
		} else{

			struct INode * inodoPath = getInodo(nroInodoPath);

			if(inodoPath->size < size + offset){
				uint32_t truncar = size + offset;
				truncarArchivo(path,truncar);
			}

			size_t bytesEscritos = 0;
			while(size > 0){
				uint32_t resto = tamanioDeBloque() - desplazamientoDentroDelBloque(offset);
				if(resto >= size){
					escribir(inodoPath,input+ bytesEscritos,size,offset);
					offset += size;
					size -= size;
				} else {
					escribir(inodoPath,input + bytesEscritos,resto,offset);
					offset += resto;
					size -= resto;
				}
                bytesEscritos += resto;
			}
		}
		pthread_rwlock_unlock(registro_archivo->lock);
	} else {
	    log_error(logger_funciones, "El archivo %s no esta abierto", path);
	    return ETXTBSY;
	}

	return resultado;
}

void escribir(struct INode * inodo, void * input,uint32_t size,uint32_t offset){
	uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
	uint32_t desplazamiento = desplazamientoDentroDelBloque(offset);
	void * ptr = posicionarme(inodo,nroBloqueLogico,desplazamiento);
	memcpy(ptr,input,size);
	input += size;
}

int32_t crearDirectorio(char * path, uint32_t mode){
	pthread_mutex_lock(&sem_crear_dir);
	int32_t resultado = 0;
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	struct INode * inodoRuta = getInodo(nroInodoRuta);
	if(nroInodoRuta != 0) {
		if(getNroInodoDeLaDireccionDelPath(path) == 0){
			resultado = agregarEntradaDirectorio(inodoRuta,ruta_separada->nombre);
			if(resultado == 0) {
                struct INode * inodoNuevoDir = getInodoDeLaDireccionDelPath(path);
                resultado = setearInodo(inodoNuevoDir,mode | S_IFDIR);

                if(resultado == 0) {
                    // nro de inodo que cargo para . en el nuevo directorio
                    uint32_t nroInodoEntradaNueva = getNroInodoDeLaDireccionDelPath(path);
                    // nro de inodo que cargo para .. en el nuevo directorio
                    uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);

                    inicializarEntradasNuevoDir(inodoNuevoDir,0,nroInodoEntradaNueva,nroInodoRuta);

                    // para actualizar el nro de directorios del grupo
                    struct Superblock *sb = read_superblock();
                    uint32_t nroGrupo = (nroInodoRuta - 1) / sb->inodes_per_group;
                    struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
                    (grupo->used_dirs_count)++;
                }

			}
		} else {
			resultado = EEXIST;
			log_error(logger_funciones, "ya existe la carpeta %s",ruta_separada->nombre);
		}
    }else{
		resultado = ENOENT;
		log_error(logger_funciones, "no existe la ruta");
	}
    free(ruta_separada->ruta);
    free(ruta_separada->nombre);
    free(ruta_separada);
	pthread_mutex_unlock(&sem_crear_dir);
	return resultado;
}

t_ruta_separada * separarPathParaNewDirEntry(char * path){
	pthread_mutex_lock(&sem_separar_path);
	t_ruta_separada * ruta_separada = malloc(sizeof(t_ruta_separada));
	ruta_separada->nombre = NULL;
	ruta_separada->ruta = NULL;
	char * separador = "/";
	if(size_array_before_split(path,separador) <= 1){
		char ** vector_ruta = string_split(path,separador);
		ruta_separada->ruta = calloc(1,1);
		if(vector_ruta[0] != NULL) {
            ruta_separada->nombre = calloc(1,strlen(vector_ruta[0]) + 1);
            strncpy(ruta_separada->nombre,vector_ruta[0],strlen(vector_ruta[0]) + 1);
		} else {
		    ruta_separada->nombre = strdup("");
		}
		string_iterate_lines(vector_ruta, (void*) free);
		free(vector_ruta);
	} else {
		char * nombre_carpeta = rindex(path,'/');
		uint32_t tamanio_nombre = strlen(nombre_carpeta);
		ruta_separada->ruta = calloc(1,strlen(path) - tamanio_nombre + 1);
		strncpy(ruta_separada->ruta,path,strlen(path) - tamanio_nombre);
		nombre_carpeta++;
		ruta_separada->nombre = calloc(1,strlen(nombre_carpeta) + 1);
		strncpy(ruta_separada->nombre,nombre_carpeta,tamanio_nombre);
	}
	pthread_mutex_unlock(&sem_separar_path);
	return ruta_separada;
}

/*
 * Agrega en las entradas de directorio de inodo, una entrada que puede ser del tipo archivo o directorio
 */
int agregarEntradaDirectorio(struct INode * inodo,char * nameNewEntry){

	uint32_t tamanio_nueva_entrada = tamanioMinEntradaDirectorio(nameNewEntry);
	uint32_t tamanio_directorio = inodo->size;
	uint32_t offset;
	int encontroEspacio = 0;
	for(offset = 0;!encontroEspacio && offset < tamanio_directorio;offset += tamanioDeBloque()){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		void * ptr = posicionarme(inodo,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanioDeBloque() > cantidad){
			struct DirEntry * directorio = leerEntrada(ptr);
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			uint32_t sizeMinEntradaDirectorioActual = tamanioMinEntradaDirectorio(nombre);
			uint32_t resto = directorio->entry_len - sizeMinEntradaDirectorioActual;
			if(resto >= tamanio_nueva_entrada){
				directorio->entry_len = sizeMinEntradaDirectorioActual;
				void * ptrInicioNuevaEntrada = ptr + sizeMinEntradaDirectorioActual;
				int errorAgregar = agregarNuevaEntrada(ptrInicioNuevaEntrada,nameNewEntry,resto);
				if(errorAgregar == 0) {
				    grabarEntrada(ptr, directorio);
                    encontroEspacio = 1;
                    free(nombre);
                    break;
				} else {
				    return errorAgregar;
				}
			}
			free(nombre);
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
			free(directorio->name);
			free(directorio);
		}
	}
	if(!encontroEspacio){
		uint32_t nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		uint32_t nroBloqueDeDato = getBloqueLibre();
		if(nroBloqueDeDato == 0) {
		    return ENOSPC;
		}
		agregarAInodo(inodo,nroBloqueLogico,nroBloqueDeDato);
		void * ptrInicioNuevaEntrada = posicionarme(inodo,nroBloqueLogico,0);
		int errorAgregar = agregarNuevaEntrada(ptrInicioNuevaEntrada,nameNewEntry,tamanioDeBloque());
		if(errorAgregar == 0) {
            inodo->size += tamanioDeBloque();
		} else {
		    return errorAgregar;
		}
	}
	return 0;
}

uint32_t tamanioMinEntradaDirectorio(char * nombre){
	uint32_t tamanio_entrada = sizeof(struct DirEntry) - sizeof(char *) + strlen(nombre);
	uint32_t resto = tamanio_entrada % 4;
	uint32_t padding = (resto == 0) ? 0 : 4 - resto;
	tamanio_entrada += padding;
	return tamanio_entrada;
}

/*
 * Desc: Agrega una nueva entrada de directorio
 * Return: número de inodo al que se asigna esta nueva entrada
 */
int agregarNuevaEntrada(void * ptrEntradaDirectorio,char * nombre, uint32_t sizeRestante){
    int error = -1;
	pthread_mutex_lock(&sem_inodo_new_dir);
	uint32_t nroInodoLibre = getInodoLibre();
	if(nroInodoLibre != 0) {
        struct DirEntry * entrada_nueva = malloc(sizeof(struct DirEntry));
	    entrada_nueva->inode = nroInodoLibre;
        entrada_nueva->entry_len = sizeRestante;
        entrada_nueva->name_len = strlen(nombre);
        entrada_nueva->name = malloc(entrada_nueva->name_len);
        memcpy(entrada_nueva->name, nombre, entrada_nueva->name_len);
        grabarEntrada(ptrEntradaDirectorio, entrada_nueva);
        struct INode * inodo = getInodo(nroInodoLibre);
        inodo->size = 0;
        error = 0;
	} else {
	    log_error(logger_funciones, "No pude agregar la nueva entrada para %s (%d size restante)", nombre, sizeRestante);
	    error = ENOSPC;
	}
	pthread_mutex_unlock(&sem_inodo_new_dir);
	return error;
}

uint32_t getInodoLibre(){
	uint32_t nroInodo = 0;
	struct Superblock *sb = read_superblock();
	if(sb->free_inodes == 0){
	    log_info(logger_funciones, "No hay inodos libres");
	} else {
		uint32_t nro_grupo;
		for(nro_grupo = 0;nroInodo == 0;nro_grupo++)
			nroInodo = getInodoLibreDelBitmap(nro_grupo);
		actualizarEstructurasCuandoPidoInodo(nroInodo);
	}
	return nroInodo;
}

uint32_t getInodoLibreDelBitmap(uint32_t nro_grupo){
	uint32_t nroInodo = 0;
	struct GroupDesc * grupo = leerGroupDescriptor(nro_grupo);
	uint32_t inicioBB = grupo->inode_bitmap;
	uint8_t *posActual = ptr_arch + inicioBB * tamanioDeBloque();
	struct Superblock *sb = read_superblock();
	int cantInodos = sb->inodes_per_group;
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

	free(ptrBit);

	return nroInodo;
}

void actualizarEstructurasCuandoPidoInodo(uint32_t nroInodo){
	struct Superblock *sb = read_superblock();
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
	free(ptrBit);
}

/*
 * Desc: Se crean las entradas . y .. para el nuevo directorio
 * nroInodoEntradaNueva -> es el nro de inodo que se carga para la entrada .
 * nroInodoRuta -> es el nro de inodo que se carga para la entrada ..
 */
void inicializarEntradasNuevoDir(struct INode * inodo, uint32_t offset, uint32_t nroInodoEntradaNueva, uint32_t nroInodoRuta){
	void * ptr = posicionarme(inodo,0,0);

	char * nombre = strdup(".");

	struct DirEntry * entrada_nueva = malloc(sizeof(struct DirEntry));
    entrada_nueva->inode = nroInodoEntradaNueva;
    uint16_t tamanio_entrada = tamanioMinEntradaDirectorio(nombre);
    entrada_nueva->entry_len = tamanio_entrada;
    entrada_nueva->name_len = strlen(nombre);
    entrada_nueva->name = malloc(entrada_nueva->name_len);
    memcpy(entrada_nueva->name, nombre, entrada_nueva->name_len);

	grabarEntrada(ptr, entrada_nueva);

	free(nombre);

	ptr = posicionarme(inodo,0,tamanio_entrada);	// el 12 es el entry_len del directorio .

	nombre = strdup("..");

	entrada_nueva = malloc(sizeof(struct DirEntry));
    entrada_nueva->inode = nroInodoEntradaNueva;
    tamanio_entrada = tamanioDeBloque() - tamanio_entrada;
    entrada_nueva->entry_len = tamanio_entrada;
    entrada_nueva->name_len = strlen(nombre);
    entrada_nueva->name = malloc(entrada_nueva->name_len);
    memcpy(entrada_nueva->name, nombre, entrada_nueva->name_len);

	grabarEntrada(ptr, entrada_nueva);

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
	    if(EXT2_INODE_HAS_MODE_FLAG(inodoDeBusqueda, S_IFDIR)) {
            nroInodoDeBusqueda = buscarNroInodoEnEntradasDirectorio(inodoDeBusqueda,ruta_separada[i]);
            if(nroInodoDeBusqueda == 0){
    // todo: revisar, entra aca cuando hago un crearDirectorio en el raiz
                log_info(logger_funciones, "No existe la ruta %s\n", path);
                break;
            }
            inodoDeBusqueda = getInodo(nroInodoDeBusqueda);
	    } else {
	        return 0;
	    }
	}
	string_iterate_lines(ruta_separada, (void*) free);
	free(ruta_separada);
	return nroInodoDeBusqueda;
}

void actualizarEstructurasCuandoPidoBloque(uint32_t nroBloqueDeDato){
	// marcar ocupado el nroBloqueDeDato dentro de su block_bitmap
	struct Superblock *sb = read_superblock();
	uint32_t nroGrupo = (nroBloqueDeDato - sb->first_data_block) / sb->blocks_per_group;
	struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
	uint32_t nroBlockBitmap = grupo->block_bitmap;
	uint8_t * inicio = posicionarInicioBloque(nroBlockBitmap);
	int cantBytes = sb->blocks_per_group / 8;
	t_bitarray 	* ptrBit = bitarray_create((char*)inicio, cantBytes);
	while(nroBloqueDeDato > sb->blocks_per_group)
		nroBloqueDeDato -= sb->blocks_per_group;
	bitarray_set_bit(ptrBit,nroBloqueDeDato - sb->first_data_block);

	(sb->free_blocks)--;	// Actualizando el free_blocks del superbloque
	(grupo->free_blocks_count)--;	// Actualizando el free_blocks del group descriptor
	free(ptrBit);
}

int32_t eliminarDirectorio(char * path){
	int32_t resultado = 0;
	struct Superblock * sb;
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

				// para actualizar el nro de directorios del grupo
				sb = read_superblock();
				uint32_t nroGrupo = (nroInodoRuta - 1) / sb->inodes_per_group;
				struct GroupDesc * grupo = leerGroupDescriptor(nroGrupo);
				(grupo->used_dirs_count)--;

			} else {
				resultado = ENOTEMPTY;
				log_error(logger_funciones, "el directorio %s contiene archivos",ruta_separada->nombre);
			}
		} else {
			resultado = ENOENT;
			log_error(logger_funciones, "no existe el directorio %s",ruta_separada->nombre);
		}
	}
	else{
		resultado = ENOENT;
		log_error(logger_funciones, "no existe la ruta");
	}
	free(ruta_separada->ruta);
	free(ruta_separada->nombre);
	free(ruta_separada);

	return resultado;
}

void eliminarEntradaDirectorio(struct INode * inodoRuta, char * nombre_entrada){
	uint32_t tamanio_directorio = inodoRuta->size;
	uint32_t offset;
	int directorioEliminado = 0;
	uint32_t tamanio_dir_eliminado;
	uint32_t nroBloqueLogico;
	uint32_t tamanioEntradaAnterior;
	void * ptr;
	for(offset = 0;!directorioEliminado && offset < tamanio_directorio;offset += tamanioDeBloque()){
		tamanioEntradaAnterior = 0;
		nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		ptr = posicionarme(inodoRuta,nroBloqueLogico,0);
		uint16_t cantidad = 0;
		while(tamanioDeBloque() > cantidad){
			struct DirEntry * directorio = leerEntrada(ptr);
			char* nombre = calloc(1, directorio->name_len + 1);
			memcpy(nombre, directorio->name, directorio->name_len);
			if(string_equals_ignore_case(nombre,nombre_entrada)){
				tamanio_dir_eliminado = directorio->entry_len;
				directorioEliminado = 1;
				free(nombre);
				free(directorio->name);
				free(directorio);
				break;
			}
			free(nombre);
			tamanioEntradaAnterior = directorio->entry_len;
			cantidad += directorio->entry_len;
			ptr += directorio->entry_len;
			free(directorio->name);
            free(directorio);
		}
	}
	// es el caso cuando una sola entrada de directorio se encuentra en un bloque
	if(tamanio_dir_eliminado == tamanioDeBloque()){
		uint32_t * ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoRuta,nroBloqueLogico);
		liberarBloque(ptrBloqueLogico);
		ptrBloqueLogico = 0;
	} else {
		ptr -= tamanioEntradaAnterior;
		struct DirEntry * directorio = leerEntrada(ptr);
		directorio->entry_len += tamanio_dir_eliminado;
		grabarEntrada(ptr, directorio);
	}
}

void liberarInodo(uint32_t nroInodoDirectorio){
	pthread_mutex_lock(&sem_liberar_inodo);
	struct INode * inodoDirEliminado = getInodo(nroInodoDirectorio);
	uint32_t tamanio = inodoDirEliminado->size;
	uint32_t offset;
	uint32_t nroBloqueLogico;
	uint32_t * ptrBloqueLogico;
// se limpia el inodo
	for(offset = 0;offset < tamanio;offset+=tamanioDeBloque()){
		nroBloqueLogico = nroBloqueDentroDelInodo(offset);
		ptrBloqueLogico = getPtrNroBloqueLogicoDentroInodo(inodoDirEliminado,nroBloqueLogico);
		liberarBloque(ptrBloqueLogico);
	}

	struct Superblock *sb = read_superblock();
	uint32_t nroGrupo = (nroInodoDirectorio - sb->first_data_block) / sb->inodes_per_group;
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
	inodoDirEliminado->mode = 0;
	inodoDirEliminado->links = 0;
	inodoDirEliminado->size = 0;
	free(ptrBit);
	pthread_mutex_unlock(&sem_liberar_inodo);

}

int directorioVacio(uint32_t nroInodoDirectorio){
	struct INode * inodoDirectorio = getInodo(nroInodoDirectorio);
	int estaVacio = 1;
	if(inodoDirectorio->size > tamanioDeBloque()){
		estaVacio = 0;
	} else {
		void * ptr;
		// me posiciono directamente en la entrada de directorio ..
		uint32_t tamanio_primer_entrada = 12;
		ptr = posicionarme(inodoDirectorio,0,12);
		struct DirEntry * directorio = leerEntrada(ptr);
		if(directorio->entry_len != tamanioDeBloque() - tamanio_primer_entrada) {
			estaVacio = 0;
		}
		free(directorio->name);
		free(directorio);
	}
	return estaVacio;
}

int32_t crearArchivo(char * path, uint32_t mode){
	int32_t resultado = 0;
	pthread_mutex_lock(&sem_crear_archivo);
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	if(nroInodoRuta != 0) {
	    struct INode * inodoDirectorio = getInodo(nroInodoRuta);
		if(getNroInodoDeLaDireccionDelPath(path) == 0){
			resultado = agregarEntradaDirectorio(inodoDirectorio,ruta_separada->nombre);
			if(resultado == 0) {
                uint32_t nroInodoEntradaNueva = getNroInodoDeLaDireccionDelPath(path);
                struct INode * inodoDeArchivo = getInodo(nroInodoEntradaNueva);
                resultado = setearInodo(inodoDeArchivo,mode | S_IFREG);
			}
		} else {
			resultado = EEXIST;
			log_error(logger_funciones, "ya existe el archivo %s",ruta_separada->nombre);
		}
	} else {
		resultado = ENOENT;
		log_error(logger_funciones, "no existe la ruta");
	}
    free(ruta_separada->ruta);
    free(ruta_separada->nombre);
    free(ruta_separada);
	pthread_mutex_unlock(&sem_crear_archivo);
	return resultado;
}

int setearInodo(struct INode * inodo, uint32_t mode){
	inodo->mode = mode;
	if(EXT2_INODE_HAS_MODE_FLAG(inodo,EXT2_S_IFDIR)){
		inodo->links = 2;
		uint32_t numeroBloqueLibre = getBloqueLibre();
		if(numeroBloqueLibre == 0) {
		    return ENOSPC;
		}
		inodo->blocks[0] = numeroBloqueLibre;
		inodo->size = tamanioDeBloque();
	} else {
		inodo->links = 1;
		inodo->size = 0;
	}
	return 0;
}

int32_t eliminarArchivo(char * path){

	int32_t resultado = 0;
	pthread_mutex_lock(&sem_eliminar_archivo);
	t_ruta_separada * ruta_separada = separarPathParaNewDirEntry(path);
	uint32_t nroInodoRuta = getNroInodoDeLaDireccionDelPath(ruta_separada->ruta);
	struct INode * inodoRuta = getInodo(nroInodoRuta);
	uint32_t nroInodoArchivo;
	if(nroInodoRuta != 0) {
		nroInodoArchivo = getNroInodoDeLaDireccionDelPath(path);
		if(nroInodoArchivo != 0){
			truncarArchivo(path,0);
			liberarInodo(nroInodoArchivo);
			eliminarEntradaDirectorio(inodoRuta,ruta_separada->nombre);

		} else {
			resultado = ENOENT;
			log_error(logger_funciones, "no existe el archivo %s",ruta_separada->nombre);
		}
	}
	else{
		resultado = ENOENT;
		log_error(logger_funciones, "no existe la ruta");
	}
	free(ruta_separada->ruta);
	free(ruta_separada->nombre);
	free(ruta_separada);
	pthread_mutex_unlock(&sem_eliminar_archivo);

	return resultado;
}

int hayEspacioSuficiente(uint32_t size){
	struct Superblock * sb = read_superblock();
	uint32_t bloquesLibres = sb->free_blocks;
	return tamanioDeBloque() * bloquesLibres >= size;
}

struct archivo_abierto * getRegistroArchivoAbierto(uint32_t nroInodo){
	struct archivo_abierto  * registro_archivo = NULL;
	int tamanio_lista = list_size(archivos_abiertos);
	int indice;
	for(indice = 0;indice < tamanio_lista;indice++){
		registro_archivo = (struct archivo_abierto *)list_get(archivos_abiertos,indice);
		if(registro_archivo->numero_inodo == nroInodo){
			break;
		}
	}
	return registro_archivo;
}




/**
 * Funciones de a bloques
 */

t_list *numerosDeBloques(struct INode *inodo, uint32_t offset, uint32_t size) {
    t_list *numeros_de_bloques = list_create();

    uint32_t nroBloqueLogicoInicio = nroBloqueDentroDelInodo(offset);
    uint32_t nroBloqueLogicoFin = nroBloqueDentroDelInodo(offset + size - 1);

    uint32_t numeroBloqueLogico;
    for(numeroBloqueLogico = nroBloqueLogicoInicio; numeroBloqueLogico <= nroBloqueLogicoFin; numeroBloqueLogico++) {
        uint32_t numero_bloque = *getPtrNroBloqueLogicoDentroInodo(inodo, numeroBloqueLogico);
        list_add(numeros_de_bloques, duplicar_uint32(numero_bloque));
    }

    return numeros_de_bloques;
}

void *traerBloqueDeCache(memcached_st *cache, uint32_t numero_bloque) {
    void *bloque = NULL;
    memcached_return_t memcached_response;
    char *key = string_from_uint32(numero_bloque);
    size_t key_length = strlen(key);
    size_t data_length;
    uint32_t flags;

    bloque = memcached_get(cache, key, key_length, &data_length, &flags, &memcached_response);

    if (memcached_response == MEMCACHED_SUCCESS) {
        log_debug(logger_funciones, "Cache hit buscando el bloque %s (%d bytes)", key, data_length);
    } else {
        bloque = NULL;
        if (data_length != tamanioDeBloque()) {
            log_error(logger_funciones, "El bloque %s existe en cache, pero tiene %d (se esperaban %d)",
                    key, data_length, tamanioDeBloque());
        } else {
            log_debug(logger_funciones, "Cache miss buscando el bloque %s", key);
        }
    }
    free(key);
    return bloque;
}

void guardarBloqueEnCache(memcached_st *cache, uint32_t numero_bloque, void *bloque) {
    char *key = string_from_uint32(numero_bloque);
    memcached_return_t memcached_response = memcached_add(cache, key, strlen(key), bloque,
            tamanioDeBloque(), (time_t)0, (uint32_t)0);

    if (memcached_response == MEMCACHED_SUCCESS) {
        log_debug(logger_funciones, "Bloque %s cacheado con exito", key);
    } else {
        log_debug(logger_funciones, "Error cacheando el bloque %s (%d bytes): %s", key,
                tamanioDeBloque(), memcached_strerror(cache, memcached_response));
    }

    free(key);
}

uint32_t numeroInodo(char *path) {
    return getNroInodoDeLaDireccionDelPath(path);
}

void bloquearLectura(uint32_t numero_inodo) {
    // FIXME: romper si no esta abierto
    struct archivo_abierto *archivo_abierto = obtener_o_crear_archivo_abierto(archivos_abiertos, numero_inodo);
    pthread_rwlock_rdlock(archivo_abierto->lock);
}

struct INode *obtenerInodo(uint32_t numero_inodo) {
    return getInodo(numero_inodo);
}

void *traerBloqueDeDisco(uint32_t numero_bloque) {
    void *bloque = NULL;
    void *buffer = obtenerBloque(numero_bloque);
    if(buffer != NULL) {
        bloque = malloc(tamanioDeBloque());
        /**
         * Duplico el bloque para que pueda ser free()able como
         * los que vienen de la cache
         */
        int indice;
        for(indice = 0; indice < tamanioDeBloque(); indice++) {
            memcpy(bloque + indice, buffer + indice, 1);
        }
    }
    return bloque;
}

void desbloquear(uint32_t numero_inodo) {
    struct archivo_abierto *archivo_abierto = obtener_o_crear_archivo_abierto(archivos_abiertos, numero_inodo);
    pthread_rwlock_unlock(archivo_abierto->lock);
}

void bloquearEscritura(uint32_t numero_inodo) {
    // FIXME: romper si no esta abierto
    struct archivo_abierto *archivo_abierto = obtener_o_crear_archivo_abierto(archivos_abiertos, numero_inodo);
    pthread_rwlock_wrlock(archivo_abierto->lock);
}

/*
 * Perdon
 */
int manejarBloquesIndireccionesParaAgregar(struct INode *inodoPath, uint32_t nroBloqueLogico) {
    uint32_t cantPtrEnIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
    uint32_t cantPtrEnIndireccionDoble = cantPtrEnIndireccionSimple * cantPtrEnIndireccionSimple;
    if(esIndireccionSimple(nroBloqueLogico) && nroBloqueLogico == 12) {
        uint32_t bloque = getBloqueLibre();
        if(bloque == 0) {
            return ENOSPC;
        }
        inodoPath->iblock = bloque;
        inodoPath->nr_blocks += tamanioDeBloque() / 512;
    }
    if(esIndireccionDoble(nroBloqueLogico)){
        if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple){
            uint32_t bloque = getBloqueLibre();
            if(bloque == 0) {
                return ENOSPC;
            }
            inodoPath->iiblock = bloque;
            inodoPath->nr_blocks += tamanioDeBloque() / 512;
        }
        // el 12 es por los bloques que ya recorrio
        if((nroBloqueLogico - 12) % cantPtrEnIndireccionSimple == 0){
            uint32_t posDentroIndireccionDoble = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple)) / cantPtrEnIndireccionSimple;
            void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
            uint32_t bloque = getBloqueLibre();
            if(bloque == 0) {
                return ENOSPC;
            }
            uint32_t nroBloqueLibre = bloque;
            inodoPath->nr_blocks += tamanioDeBloque() / 512;
            uint32_t * ptrNroBloqueLibre = &nroBloqueLibre;
            memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
        }
    }
    if(esIndireccionTriple(nroBloqueLogico)){
        if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble){
            uint32_t bloque = getBloqueLibre();
            if(bloque == 0) {
                return ENOSPC;
            }
            inodoPath->iiiblock = bloque;
            inodoPath->nr_blocks += tamanioDeBloque() / 512;
        }
        uint32_t nroBloqueLibre;
        if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionDoble == 0){
            uint32_t posDentroIndireccionTriple = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) / cantPtrEnIndireccionDoble;
            void * ptrBloque = desplazarme(inodoPath->iiiblock,posDentroIndireccionTriple * 4);
            nroBloqueLibre = getBloqueLibre();
            if(nroBloqueLibre == 0) {
                return ENOSPC;
            }
            uint32_t * ptrNroBloqueLibre = &nroBloqueLibre;
            memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
            inodoPath->nr_blocks += tamanioDeBloque() / 512;
        }
        if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionSimple == 0){
            void * ptrIndTriple = getPtrBloqueIndTriple(inodoPath->iiiblock,nroBloqueLogico);
            uint32_t * nroBloqueIndTriple = (uint32_t *)ptrIndTriple;
            uint32_t nroBloqueRelativo = nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble);
            while(nroBloqueRelativo >= cantPtrEnIndireccionDoble)
                nroBloqueRelativo -= cantPtrEnIndireccionDoble;
            uint32_t posDentroIndireccionDoble = nroBloqueRelativo / cantPtrEnIndireccionSimple;
            void * ptrBloque = desplazarme(*nroBloqueIndTriple,posDentroIndireccionDoble * 4);
            uint32_t nroBloqueLibreIndDoble = getBloqueLibre();
            if(nroBloqueLibreIndDoble == 0) {
                return ENOSPC;
            }
            uint32_t * ptrNroBloqueLibre = &nroBloqueLibreIndDoble;
            memcpy(ptrBloque,ptrNroBloqueLibre,sizeof(uint32_t));
            inodoPath->nr_blocks += tamanioDeBloque() / 512;
        }
    }
    return 0;
}

int cantidadDeBloques(struct INode *inodo) {
    return inodo->nr_blocks * 512 / tamanioDeBloque();
}

/**
 * Busca un bloque libre y lo agrega al final del inodo, actualizando
 * cantidad de bloques del inodo
 */
int agregarBloqueNuevo(struct INode *inodo) {
    manejarBloquesIndireccionesParaAgregar(inodo, cantidadDeBloques(inodo));
    uint32_t bloqueLibre = getBloqueLibre();
    if(bloqueLibre == 0) {
        return ENOSPC;
    }
    uint32_t *punteroNumeroBloque = getPtrNroBloqueLogicoDentroInodo(inodo, cantidadDeBloques(inodo));
    if(punteroNumeroBloque == NULL) {
        return EFBIG;
    }
    *punteroNumeroBloque = bloqueLibre;
    // nr_blocks es cantidad de bloques de 512b
    inodo->nr_blocks += tamanioDeBloque() / 512;
    return 0;
}

void manejarBloquesIndireccionesParaEliminar(struct INode *inodoPath, uint32_t nroBloqueLogico) {
    uint32_t cantPtrEnIndireccionSimple = tamanioDeBloque() / sizeof(uint32_t);
    uint32_t cantPtrEnIndireccionDoble = cantPtrEnIndireccionSimple * cantPtrEnIndireccionSimple;
    if(esIndireccionSimple(nroBloqueLogico) && nroBloqueLogico == 12){
        liberarBloque(&inodoPath->iblock);
        inodoPath->nr_blocks -= tamanioDeBloque() / 512;
    }
    if(esIndireccionDoble(nroBloqueLogico)){
        // el 12 es por los bloques que ya recorrio
        if((nroBloqueLogico - 12) % cantPtrEnIndireccionSimple == 0){
            uint32_t posDentroIndireccionDoble = (nroBloqueLogico - (12 + cantPtrEnIndireccionSimple)) / cantPtrEnIndireccionSimple;
            void * ptrBloque = desplazarme(inodoPath->iiblock,posDentroIndireccionDoble * 4);
            uint32_t * nroBloque = (uint32_t *)ptrBloque;
            liberarBloque(nroBloque);
            inodoPath->nr_blocks -= tamanioDeBloque() / 512;
        }
        if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple){
            liberarBloque(&inodoPath->iiblock);
            inodoPath->nr_blocks -= tamanioDeBloque() / 512;
        }
    }
    if(esIndireccionTriple(nroBloqueLogico)){
        void * ptrIndDoble = getPtrBloqueIndDoble(inodoPath->iiiblock,nroBloqueLogico);
        uint32_t * nroBloqueIndDoble = (uint32_t *)ptrIndDoble;
        void * ptrIndTriple = getPtrBloqueIndTriple(inodoPath->iiiblock,nroBloqueLogico);
        uint32_t * nroBloqueIndTriple = (uint32_t *)ptrIndTriple;
        if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionSimple == 0){
            liberarBloque(nroBloqueIndDoble);
            inodoPath->nr_blocks -= tamanioDeBloque() / 512;
        }
        if((nroBloqueLogico - (12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble)) % cantPtrEnIndireccionDoble == 0){
            liberarBloque(nroBloqueIndTriple);
            inodoPath->nr_blocks -= tamanioDeBloque() / 512;
        }
        if(nroBloqueLogico == 12 + cantPtrEnIndireccionSimple + cantPtrEnIndireccionDoble){
            liberarBloque(&inodoPath->iiiblock);
            inodoPath->nr_blocks -= tamanioDeBloque() / 512;
        }
    }
}

/**
 * Remueve un bloque de un inodo y lo marca como libre.
 */
int removerUltimoBloque(struct INode *inodo) {
    if(!inodo->nr_blocks) {
        return ESPIPE;
    }
    uint32_t *punteroNumeroBloque = getPtrNroBloqueLogicoDentroInodo(inodo, (cantidadDeBloques(inodo) - 1));
    if(punteroNumeroBloque == NULL) {
        return EFAULT;
    }
    liberarBloque(punteroNumeroBloque);
    inodo->nr_blocks -= tamanioDeBloque() / 512;

    // achica el size al multiplo del bloque anterior a este
    inodo->size = ((inodo->size - 1) / tamanioDeBloque()) * tamanioDeBloque();
    return 0;
}

void grabarInodo(uint32_t numero_inodo, struct INode *inodo) {
    struct INode *punteroInodoDisco = getInodo(numero_inodo);
    memcpy(punteroInodoDisco, inodo, sizeof(struct INode));
}

int truncar(char *path, size_t size) {
    uint32_t numero_inodo = numeroInodo(path);

    struct INode *inodoArchivo = obtenerInodo(numero_inodo);
    bloquearEscritura(numero_inodo);

    int codigo_error = 0;
    if(size < inodoArchivo->size) {
        int cantidad_bloques_final = nroBloqueDentroDelInodo(size - 1) + 1;

        while(!codigo_error && ( cantidadDeBloques(inodoArchivo) > cantidad_bloques_final)) {
            codigo_error = removerUltimoBloque(inodoArchivo);
        }
    }

    if(!codigo_error) {
        inodoArchivo->size = size;
    }

    grabarInodo(numero_inodo, inodoArchivo);
    desbloquear(numero_inodo);

    return codigo_error;
}
