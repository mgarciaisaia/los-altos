#ifndef MEMCACHED_UTILS_H_
#define MEMCACHED_UTILS_H_

#include "src/nipc/nipc.h"
#include <libmemcached/memcached.h>
#include "src/commons/log.h"

void set_memcached_utils_logger(t_log *logger);

struct nipc_packet *query_memcached(memcached_st *cache, const char *path, enum tipo_nipc nipc_type, uint32_t client_id);

void store_memcached(memcached_st *cache, const char *path, struct nipc_packet *packet);

void delete_memcached(memcached_st *cache, const char* path, enum tipo_nipc nipc_type);

#endif /* MEMCACHED_UTILS_H_ */
