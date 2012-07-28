#ifndef MEMCACHED_UTILS_H_
#define MEMCACHED_UTILS_H_

#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"

struct memcached_response{
	uint32_t nroBloqueInicial;
	uint32_t nroBloqueFinal;
	uint32_t cantBloques;
//	uint8_t * ptrDatos;

};
typedef struct memcached_response memcached_response;

void set_memcached_utils_logger(t_log *logger);

memcached_response *transform_query(const char *path,uint32_t offset,uint32_t size);

char * query_memcached(memcached_st *cache,uint32_t nroBloque);

void store_memcached(memcached_st *cache, uint32_t nroBloque, void * buffer);

int32_t almacenar_memcached(memcached_st *cache,char *path, uint32_t offset,
		uint32_t size, void *buffer);

uint32_t read_from_memcached(memcached_st *cache,char *path,
		uint32_t offset, uint32_t size,void *buffer);

#endif /* MEMCACHED_UTILS_H_ */
