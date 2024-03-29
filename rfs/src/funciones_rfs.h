/*
 * funciones_rfs.h
 *
 *  Created on: 03/05/2012
 *      Author: utnso
 */


#ifndef FUNCIONES_RFS_H_
#define FUNCIONES_RFS_H_

#include <string.h>
#include <stdint.h>
#include <src/commons/collections/list.h>
#include <src/commons/collections/queue.h>
//#include <commons/collections/queue.h>
#include "src/commons/log.h"
#include <libmemcached/memcached.h>
#include "../commons/src/commons/misc.h"
#include "../commons/src/commons/string.h"


#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000
#define EXT2_INODE_HAS_MODE_FLAG(inode, flag)	((inode->mode & 0xF000) == flag)

	struct Superblock
	{
	    uint32_t inodes;
	    uint32_t blocks;
	    uint32_t reserved_blocks;
	    uint32_t free_blocks;
	    uint32_t free_inodes;
	    uint32_t first_data_block;
	    uint32_t log_block_size;
	    uint32_t fragment_size;
	    uint32_t blocks_per_group;
	    uint32_t fragments_per_group;
	    uint32_t inodes_per_group;
	    uint32_t mount_time;
	    uint32_t write_time;
	    uint16_t mount_count;
	    uint16_t max_mount;
	    uint16_t magic;
	    uint16_t state;
	    uint16_t errors;
	    uint16_t minor;
	    uint32_t last_check;
	    uint32_t check_int;
	    uint32_t creator_os;
	    uint32_t rev_level;
	    uint16_t resv_uid;
	    uint16_t resv_gid;
	    uint32_t first_inode;
	    uint16_t inode_size;
	    uint16_t block_group;
	    uint32_t feature_compat;
	    uint32_t feature_incompat;
	    uint32_t feature_ro_compat;
	    uint8_t uuid[16];

	};

	struct GroupDesc
	{
	    uint32_t block_bitmap;
	    uint32_t inode_bitmap;
	    uint32_t inode_table;
	    uint16_t free_blocks_count;
	    uint16_t free_inodes_count;
	    uint16_t used_dirs_count;
	    uint16_t dummy[7];
	};

	struct INode
	{
	    uint16_t mode;
	    uint16_t uid;	// generar ??
	    uint32_t size;
	    uint32_t atime;
	    uint32_t ctime;
	    uint32_t mtime;
	    uint32_t dtime;
	    uint16_t gid;	// ??
	    uint16_t links;
	    uint32_t nr_blocks;	// ver que es
	    uint32_t flags;	// ??
	    uint32_t reserved1;
	    uint32_t blocks[12];
	    uint32_t iblock;
	    uint32_t iiblock;
	    uint32_t iiiblock;
	    uint32_t generation;	// ??
	    uint32_t file_acl;	// = 0
	    uint32_t size_hi;	// ??
	    uint32_t reserved2[4];	// ??
	};

	struct DirEntry
	{
	    uint32_t inode;
	    uint16_t entry_len;
	    uint8_t name_len;
	    uint8_t type;
	    char *name;
	};

	typedef struct readdir_stat
	{
	    uint16_t mode;
	    uint16_t nlink;
	    uint16_t uid;
	    uint16_t gid;
//	    __dev_t st_rdev;
		char * name;
	} t_readdir_stat;

	typedef struct ruta_separada{
		char * ruta;
		char * nombre;
	} t_ruta_separada;

	typedef struct {
		uint32_t nroInodo;
		pthread_rwlock_t * semaforo_rw;
		int referencias;
	} t_nodo_archivo;

	void set_logger_funciones(t_log *logger_para_funciones);
	void init_semaforos(void);
	void destroy_semaforos(void);

	void mapear_archivo(char *);
	struct Superblock *read_superblock();

	uint32_t cantidadDeGrupos();

	struct GroupDesc * leerGroupDescriptor(uint32_t);

	uint32_t nroBloqueInicioDeGrupo(uint32_t);
	uint32_t nroInodoInicioDeGrupo(uint32_t);

	struct INode * getInodo(uint32_t);
	void *obtenerBloque(uint32_t numero_bloque);
	uint8_t * posicionarInicioBloque(uint32_t);

	uint32_t buscarNroInodoEnEntradasDirectorio(struct INode * inodoDeBusqueda,char * ruta);

	t_list *listarDirectorio(char *);
	t_list *cargarEntradasDirectorioALista(struct INode *);

	uint32_t nroBloqueDentroDelInodo(uint32_t offset);
	uint32_t desplazamientoDentroDelBloque(uint32_t offset);

	void * posicionarme(struct INode * inodo,uint32_t nroBloqueLogico,uint32_t desplazamiento);
	void * desplazarme(uint32_t nroBloqueDeDato,uint32_t desplazamiento);

	int esBloqueDirecto(uint32_t nroBloque);
	int esIndireccionSimple(uint32_t nroBloque);
	int esIndireccionDoble(uint32_t nroBloque);
	int esIndireccionTriple(uint32_t nroBloque);

	uint32_t * posicionarIndireccionSimple(uint32_t, uint32_t);
	uint32_t * posicionarIndireccionDoble(uint32_t, uint32_t);
	uint32_t * posicionarIndireccionTriple(uint32_t, uint32_t);

	size_t leerArchivo(char * path, uint32_t offset, uint32_t bytesALeer, void **bufferPointer);

	struct INode * getInodoDeLaDireccionDelPath(char * path);
	int32_t truncarArchivo(char * path,uint32_t offset);
	uint32_t * getPtrNroBloqueLogicoDentroInodo(struct INode * inodo,uint32_t nroBloqueLogico);
	void liberarBloque(uint32_t * ptrBloqueDeDatos);
	void * getPtrBloqueIndDoble(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico);
	void * getPtrBloqueIndTriple(uint32_t nroBloqueIndireccion,uint32_t nroBloqueLogico);

	uint32_t getBloqueLibre(void);
	uint32_t getBloqueLibreDelBitmap(uint32_t nro_grupo);
	void agregarAInodo(struct INode *,uint32_t ,uint32_t);

	void completarBloque(struct INode * inodo, uint32_t offset, uint32_t size);
	void escribirBloque(void *, uint32_t);

	int32_t escribirArchivo(char * path, void * input, uint32_t size, uint32_t offset);
	void escribir(struct INode * inodo, void * input,uint32_t size,uint32_t offset);

	int32_t crearDirectorio(char * path, uint32_t mode);
	t_ruta_separada * separarPathParaNewDirEntry(char * path);
	int agregarEntradaDirectorio(struct INode * inodo,char * nombre);
	int agregarNuevaEntrada(void * ptrEntradaDirectorio,char * nombre, uint32_t sizeRestante);
	uint32_t tamanioMinEntradaDirectorio(char * nombre);
	uint32_t getInodoLibre(void);
	void actualizarEstructurasCuandoPidoInodo(uint32_t nroInodo);
	uint32_t getInodoLibreDelBitmap(uint32_t nro_grupo);

	void inicializarEntradasNuevoDir(struct INode * inodo, uint32_t offset, uint32_t nroInodoEntradaNueva, uint32_t nroInodoRuta);

	uint32_t getNroInodoDeLaDireccionDelPath(char * path);
	void actualizarEstructurasCuandoPidoBloque(uint32_t nroBloqueDeDato);

	int32_t eliminarDirectorio(char * path);
	void eliminarEntradaDirectorio(struct INode * inodoRuta, char * nombre_entrada);
	void liberarInodo(uint32_t nroInodoDirectorio);
	int directorioVacio(uint32_t nroInodoDirectorio);

	int32_t crearArchivo(char * path, uint32_t mode);
	int setearInodo(struct INode * inodoArchivo, uint32_t mode);

	int32_t eliminarArchivo(char * path);

	int hayEspacioSuficiente(uint32_t size);

	struct archivo_abierto * getRegistroArchivoAbierto(uint32_t nroInodo);

	int tamanioDeBloque();

	void *traerBloqueDeDisco(uint32_t numero_bloque);


	/**
	 * Funciones de a bloques
	 */

	t_list *numerosDeBloques(struct INode *inodo, uint32_t offset, uint32_t size);

	void *traerBloqueDeCache(memcached_st *cache, uint32_t numero_bloque);

	void guardarBloqueEnCache(memcached_st *cache, uint32_t numero_bloque, void *bloque);

	uint32_t numeroInodo(char *path);

	void bloquearLectura(uint32_t numero_inodo);

	struct INode *obtenerInodo(uint32_t numero_inodo);

	void *traerBloqueDeDisco(uint32_t numero_bloque);

	void desbloquear(uint32_t numero_inodo);

	void bloquearEscritura(uint32_t numero_inodo);

	void *traerBloque(uint32_t numero_bloque);

	void grabarBloque(uint32_t numero_bloque, void *bloque);

	int cantidadDeBloques(struct INode *inodo);

	/**
	 * Busca un bloque libre y lo agrega al final del inodo, actualizando
	 * cantidad de bloques del inodo
	 */
	int agregarBloqueNuevo(struct INode *inodo);

	/**
	 * Remueve un bloque de un inodo y lo marca como libre.
	 */
	int removerUltimoBloque(struct INode *inodo);

	void grabarInodo(uint32_t numero_inodo, struct INode *inodo);

	int truncar(char *path, size_t size);

#endif /* FUNCIONES_RFS_H_ */
