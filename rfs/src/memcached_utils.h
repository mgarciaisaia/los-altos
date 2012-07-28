#ifndef MEMCACHED_UTILS_H_
#define MEMCACHED_UTILS_H_

#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"

struct memcached_response{
	uint32_t nroBloque;
	uint8_t * ptrDatos;
};
typedef struct memcached_response memcached_response;

void set_memcached_utils_logger(t_log *logger);


char *key_for_memcached(const char *path, char operation);

memcached_response transform_query(const char *path,uint32_t offset);

void query_memcached(memcached_st *cache, const char *path,char operation,uint32_t offset);

void store_memcached(memcached_st *cache, const char *path,
		uint32_t size, uint32_t offset, char operation);

void delete_memcached(memcached_st *cache, const char* path, uint32_t offset);

#endif /* MEMCACHED_UTILS_H_ */
