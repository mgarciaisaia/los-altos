/*
 * our_engine.h
 *
 *  Created on: 12/05/2012
 *      Author: utnso
 */

#ifndef OUR_ENGINE_H_
#define OUR_ENGINE_H_

#include <memcached/engine.h>
#include <memcached/util.h>
#include <memcached/visibility.h>
#include <time.h>

struct key_element {
	char *key;
	size_t nkey;
	bool stored;
	size_t data_size;
	void *data;
	int32_t flags;
	bool libre;
	rel_time_t exptime;
	struct timespec tp;
};

typedef struct key_element key_element;

/*
 * Esta es una estructura custom que utilizo para almacenar la configuración que me pasa memcached
 */
typedef struct {
	size_t cache_max_size;
	size_t block_size_max;
	size_t chunk_size;
} t_dummy_ng_config;


/*
 * Esta es la estructura que utilizo para representar el engine, para que memcached pueda manipularla el
 * primer campo de esta tiene que ser ENGINE_HANDLE_V1 engine; el resto de los campos pueden ser los que querramos
 */
typedef struct {
	ENGINE_HANDLE_V1 engine;
	GET_SERVER_API get_server_api;
	t_dummy_ng_config config;
} t_dummy_ng;

// Esta funcion es escencial ya que es la que busca memcached para ejecutar cuando levanta la shared library
MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance(uint64_t interface,
		GET_SERVER_API get_server_api, ENGINE_HANDLE **handle);

#endif /* OUR_ENGINE_H_ */
