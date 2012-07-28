#include "memcached_uses.h"
#include <stdlib.h>
#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"
#include "funciones_rfs.h"
#include "src/commons/string.h"

#define BLOQUE_SIZE 1024
t_log *memcached_logger;

void set_memcached_utils_logger(t_log *logger) {
	memcached_logger = logger;
}

/*char *key_for_memcached(const char *path, char operation) {
//	char *operation = NULL;
//	if (operacion == nipc_getattr_response) {
//		operation = "R"; // read
//	} else {
//		operation = "W"; // write
//	}
	char *key = malloc(strlen(path) + 2);
	strncpy(key, operation, 1);
	strcpy(key + 1, path);
	return key;
}
*/
memcached_response *transform_query(const char *path,uint32_t offset,uint32_t size){

	memcached_response *respuesta = malloc(sizeof (memcached_response));
	struct INode * inodoI = getInodoDeLaDireccionDelPath((char*)path);
	uint32_t nroBloqueLogicoI = nroBloqueDentroDelInodo(offset);
	uint32_t * bloqueFisicoI =getPtrNroBloqueLogicoDentroInodo(inodoI,nroBloqueLogicoI);
	respuesta->nroBloqueInicial = *bloqueFisicoI;

	struct INode * inodoF = getInodoDeLaDireccionDelPath((char*)path);
	uint32_t nroBloqueLogicoF = nroBloqueDentroDelInodo(offset+size -1);
	uint32_t * bloqueFisicoF =getPtrNroBloqueLogicoDentroInodo(inodoF,nroBloqueLogicoF);
	respuesta->nroBloqueFinal = *bloqueFisicoF;

	respuesta->cantBloques = respuesta->nroBloqueFinal - respuesta->nroBloqueInicial;
	//aca para los datos
//	respuesta->ptrDatos = posicionarInicioBloque(respuesta->nroBloque);

	return respuesta;
}

//que la llame adentro del leerArchivo
char * query_memcached(memcached_st *cache,uint32_t nroBloque){

	char* cached_data = malloc(BLOQUE_SIZE);
	memcached_return_t memcached_response;
	char *key = string_from_uint32(nroBloque);
	size_t key_length = strlen(key) + 1;
	size_t data_length;
	uint32_t flags;

	cached_data = memcached_get(cache, key, key_length, &data_length,
			&flags, &memcached_response);

	if (memcached_response == MEMCACHED_SUCCESS) {
		log_debug(memcached_logger, "Cache hit buscando la clave %s (%d bytes)",
				key, data_length);

		return cached_data;

	} else {
		log_debug(memcached_logger, "Cache miss buscando la clave %s", key);
		return NULL;
	}

	free(key);
	return 0;
}

void store_memcached(memcached_st *cache, uint32_t nroBloque, void * buffer) {

//	memcached_response *respuesta = transform_query(path,offset);
	char *key = string_from_uint32(nroBloque);
	//	char *key = key_for_memcached(path, operation);
	memcached_return_t memcached_response = memcached_add(cache, key,
			strlen(key), buffer, BLOQUE_SIZE, (time_t) 0,
			(uint32_t) 0);

	if (memcached_response == MEMCACHED_SUCCESS) {
		log_info(memcached_logger, "Se guardo la clave %s", key);
	} else {
		log_error(memcached_logger, "Error guardando datos en la clave %s: %s",
				key, memcached_strerror(cache, memcached_response));
	}
//	free(key);
//	free(respuesta);
}

void delete_memcached(memcached_st *cache, const char* path, uint32_t offset) {

	//calcular el tamaño del archivo asi se cuantos bloques ocupa
	struct INode * inodo = getInodoDeLaDireccionDelPath((char*)path);

	memcached_response *respuesta = transform_query(path,offset,inodo->size);
	char *key;
//	uint32_t i = 0;
		uint32_t bloqueBorrado = respuesta->nroBloqueInicial;
		while(bloqueBorrado <= respuesta->nroBloqueFinal){

			key = string_from_uint32(bloqueBorrado);
			memcached_return_t memcached_response = memcached_delete(cache, key,
					strlen(key), (time_t) 0);
			if (memcached_response == MEMCACHED_SUCCESS) {
				log_info(memcached_logger, "Se borro la clave %s", key);
			} else {
				log_error(memcached_logger, "Error borrando la clave %s: %s", key,
						memcached_strerror(cache, memcached_response));
			}

			bloqueBorrado++;
		}
}

int32_t almacenar_memcached(memcached_st *cache,char *path, uint32_t offset,
		uint32_t size, void *buffer){

	int32_t bytesEscritos = -1;
	memcached_response *respuesta = transform_query(path,offset,size);

	uint32_t i = 0;
	uint32_t bloqueEscrito = respuesta->nroBloqueInicial;
	while(bloqueEscrito <= respuesta->nroBloqueFinal){

		store_memcached(cache, bloqueEscrito,buffer+i);

		i=+BLOQUE_SIZE;
		bloqueEscrito++;
	}

	if ((bloqueEscrito* BLOQUE_SIZE)>0)
		bytesEscritos = 0;

	return bytesEscritos;

}

uint32_t read_from_memcached(memcached_st *cache,char *path,
		uint32_t offset, uint32_t size,void *buffer){

	uint32_t bytesLeidos = 0;
	memcached_response *respuesta = transform_query(path,offset,size);

	buffer = malloc(size);
	char *datos_leidos;
	uint32_t i = 0;
    size_t bloquesLeidos = respuesta->nroBloqueInicial;

    while(bloquesLeidos <= respuesta->nroBloqueFinal){

    	datos_leidos = query_memcached(cache, bloquesLeidos);

    	if (datos_leidos != NULL)
    	memcpy(buffer + i, datos_leidos, BLOQUE_SIZE);

    		i=+BLOQUE_SIZE;
    		bloquesLeidos++;
    }

    free(datos_leidos);

        bytesLeidos = bloquesLeidos * BLOQUE_SIZE;
        return bytesLeidos;
}


