#include "memcached_utils.h"
#include <stdlib.h>
#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"
#define BLOQUE_SIZE 1024
t_log *memcached_logger;

void set_memcached_utils_logger(t_log *logger) {
	memcached_logger = logger;
}

char *key_for_memcached(const char *path, char operation) {
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

memcached_response transform_query(const char *path,uint32_t offset){

	memcached_response *respuesta = malloc(sizeof (memcached_response));
	struct INode * inodo = getInodoDeLaDireccionDelPath(path);
	uint32_t nroBloqueLogico = nroBloqueDentroInodo(offset);
	uint32_t * bloqueFisico =getPtrNroBloqueLogicoDentroInodo(inodo,nroBloqueLogico);
	respuesta->nroBloque = *bloqueFisico;

	//aca para los datos
	respuesta->ptrDatos = posicionarInicioBloque(respuesta->nroBloque);

	return respuesta;
}

//que la llame adentro del leerArchivo
void query_memcached(memcached_st *cache, const char *path,char operation,uint32_t offset){

	memcached_response *respuesta = transform_query(path,offset);
	memcached_return_t memcached_response;
//	char *key = key_for_memcached(path, operation);
	char* key = (char *)respuesta->nroBloque;
	size_t key_length = strlen(key) + 1;
	size_t data_length;
	uint32_t flags;

	char *cached_data = memcached_get(cache, key, key_length, &data_length,
			&flags, &memcached_response);

	if (memcached_response == MEMCACHED_SUCCESS) {
		log_debug(memcached_logger, "Cache hit buscando la clave %s (%d bytes)",
				key, data_length);

		//responder algo q lo hizo bien

	} else {
		log_debug(memcached_logger, "Cache miss buscando la clave %s", key);
	}

	free(key);
	free(respuesta);
//	return response;
}

void store_memcached(memcached_st *cache, const char *path,
		uint32_t size, uint32_t offset, char operation) {

	memcached_response *respuesta = transform_query(path,offset);
	char *key = (char*)respuesta->nroBloque;
	//	char *key = key_for_memcached(path, operation);
	memcached_return_t memcached_response = memcached_add(cache, key,
			strlen(key), *(respuesta->ptrDatos), BLOQUE_SIZE, (time_t) 0,
			(uint32_t) 0);

	if (memcached_response == MEMCACHED_SUCCESS) {
		log_info(memcached_logger, "Se guardo la clave %s", key);
	} else {
		log_error(memcached_logger, "Error guardando datos en la clave %s: %s",
				key, memcached_strerror(cache, memcached_response));
	}
	free(key);
	free(respuesta);
}

void delete_memcached(memcached_st *cache, const char* path, uint32_t offset) {

	memcached_response *respuesta = transform_query(path,offset);
	char *key = (char*)respuesta->nroBloque;
	memcached_return_t memcached_response = memcached_delete(cache, key,
			strlen(key), (time_t) 0);
	if (memcached_response == MEMCACHED_SUCCESS) {
		log_info(memcached_logger, "Se borro la clave %s", key);
	} else {
		log_error(memcached_logger, "Error borrando la clave %s: %s", key,
				memcached_strerror(cache, memcached_response));
	}

}
