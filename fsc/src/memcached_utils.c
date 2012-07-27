#include "memcached_utils.h"
#include <stdlib.h>
#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"

t_log *memcached_logger;

void set_memcached_utils_logger(t_log *logger) {
	memcached_logger = logger;
}

char *key_for_memcached(const char *path, enum tipo_nipc nipc_type) {
	char *operation = NULL;
	if (nipc_type == nipc_getattr_response) {
		operation = "A"; // attribute
	} else {
		operation = "D"; // directory
	}
	char *key = malloc(strlen(path) + 2);
	strncpy(key, operation, 1);
	strcpy(key + 1, path);
	return key;
}

struct nipc_packet *query_memcached(memcached_st *cache, const char *path,
		enum tipo_nipc nipc_type, uint32_t client_id) {
	struct nipc_packet *response = NULL;
	memcached_return_t memcached_response;
	char *key = key_for_memcached(path, nipc_type);
	size_t key_length = strlen(key) + 1;
	size_t data_length;
	uint32_t flags;

	char *cached_data = memcached_get(cache, key, key_length, &data_length,
			&flags, &memcached_response);

	if (memcached_response == MEMCACHED_SUCCESS) {
		log_debug(memcached_logger, "Cache hit buscando la clave %s (%d bytes)",
				key, data_length);
		response = malloc(sizeof(struct nipc_packet));
		response->client_id = client_id;
		response->type = nipc_type;
		response->data = cached_data;
		response->data_length = data_length;
	} else {
		log_debug(memcached_logger, "Cache miss buscando la clave %s", key);
	}

	free(key);
	return response;
}

void store_memcached(memcached_st *cache, const char *path,
		struct nipc_packet *packet) {
	char *key = key_for_memcached(path, packet->type);
	memcached_return_t memcached_response = memcached_add(cache, key,
			strlen(key), packet->data, packet->data_length, (time_t) 0,
			(uint32_t) 0);
	if (memcached_response == MEMCACHED_SUCCESS) {
		log_info(memcached_logger, "Se guardaron %d bytes para la clave %s",
				packet->data_length, key);
	} else {
		log_error(memcached_logger, "Error guardando datos en la clave %s: %s",
				key, memcached_strerror(cache, memcached_response));
	}
	free(key);
}
void delete_memcached(memcached_st *cache, const char* path, enum tipo_nipc nipc_type) {

	char *key = key_for_memcached(path, nipc_type);
	memcached_return_t memcached_response = memcached_delete(cache, key,
			strlen(key), (time_t) 0);
	if (memcached_response == MEMCACHED_SUCCESS) {
		log_info(memcached_logger, "Se borro la clave %s", key);
	} else {
		log_error(memcached_logger, "Error borrando la clave %s: %s", key,
				memcached_strerror(cache, memcached_response));
	}

}
