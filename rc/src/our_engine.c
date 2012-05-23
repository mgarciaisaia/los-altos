/*
 * our_engine.c
 *
 *  Created on: 12/05/2012
 *      Author: utnso
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>

// Este include es necesario para que el include <memcached/config_parser.h> no tire error de compilación
#include <stdbool.h>
// Aca estan las tools de memcached para levantar la configuración provista por el usuario en los parametros de ejecución
#include <memcached/config_parser.h>

#include "our_engine.h"
#include <mcheck.h>
#include <sys/mman.h>
#include <unistd.h>
#include <err.h>

	key_element key_vector[];

/*
 * Estas son las funciones estaticas necesarias para que el engine funcione
 */

static ENGINE_ERROR_CODE dummy_ng_initialize(ENGINE_HANDLE* , const char* config_str);
static void dummy_ng_destroy(ENGINE_HANDLE*, const bool force);

static ENGINE_ERROR_CODE dummy_ng_allocate(ENGINE_HANDLE* , const void* cookie, item **item, const void* key,
											const size_t nkey, const size_t nbytes, const int flags, const rel_time_t exptime);
static bool dummy_ng_get_item_info(ENGINE_HANDLE *, const void *cookie, const item* item, item_info *item_info);
static ENGINE_ERROR_CODE dummy_ng_store(ENGINE_HANDLE* , const void *cookie, item* item, uint64_t *cas,
											ENGINE_STORE_OPERATION operation, uint16_t vbucket);
static void dummy_ng_item_release(ENGINE_HANDLE* , const void *cookie, item* item);
static ENGINE_ERROR_CODE dummy_ng_get(ENGINE_HANDLE* , const void* cookie, item** item, const void* key, const int nkey, uint16_t vbucket);

static ENGINE_ERROR_CODE dummy_ng_flush(ENGINE_HANDLE* , const void* cookie, time_t when);

static ENGINE_ERROR_CODE dummy_ng_item_delete(ENGINE_HANDLE* , const void* cookie, const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket);

/*
 * ************************** Dummy Functions **************************
 *
 * Estas funciones son dummy, son necesarias para que el engine las tengas
 * pero no tienen logica alguna y no seran necesarias implementar
 *
 */

static const engine_info* dummy_ng_get_info(ENGINE_HANDLE* );
static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* , const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat);
static void dummy_ng_reset_stats(ENGINE_HANDLE* , const void *cookie);
static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* , const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response);
static void dummy_ng_item_set_cas(ENGINE_HANDLE *, const void *cookie, item* item, uint64_t val);

/**********************************************************************/


/*
 * Esta función es la que va a ser llamada cuando se reciba una signal
 */
void dummy_ng_dummp(int signal);

/*
 * Esta va a ser la estructura donde voy a ir almacenando la información que me llegue.
 */

void *cache;
key_element key_vector[];

/*
 * Esta es la función que va a llamar memcached para instanciar nuestro engine
 */
MEMCACHED_PUBLIC_API ENGINE_ERROR_CODE create_instance(uint64_t interface, GET_SERVER_API get_server_api, ENGINE_HANDLE **handle) {

	if (interface == 0) {
		return ENGINE_ENOTSUP;
	}

	t_dummy_ng *engine = calloc(1, sizeof(t_dummy_ng));
	if (engine == NULL) {
		return ENGINE_ENOMEM;
	}

	engine->engine.interface.interface = 1;

	/*
	 * La API de memcache funciona pasandole a la estructura engine que esta dentro de nuestra
	 * estructura t_dummy_ng los punteros a las funciónes necesarias.
	 */
	engine->engine.initialize = dummy_ng_initialize;
	engine->engine.destroy = dummy_ng_destroy;
	engine->engine.get_info = dummy_ng_get_info;
	engine->engine.allocate = dummy_ng_allocate;
	engine->engine.remove = dummy_ng_item_delete;
	engine->engine.release = dummy_ng_item_release;
	engine->engine.get = dummy_ng_get;
	engine->engine.get_stats = dummy_ng_get_stats;
	engine->engine.reset_stats = dummy_ng_reset_stats;
	engine->engine.store = dummy_ng_store;
	engine->engine.flush = dummy_ng_flush;
	engine->engine.unknown_command = dummy_ng_unknown_command;
	engine->engine.item_set_cas = dummy_ng_item_set_cas;
	engine->engine.get_item_info = dummy_ng_get_item_info;


	engine->get_server_api = get_server_api;

	/*
	 * memcached solo sabe manejar la estructura ENGINE_HANDLE
	 * el cual es el primer campo de nuestro t_dummy_ng
	 * El puntero de engine es igual a &engine->engine
	 *
	 * Retornamos nuestro engine a traves de la variable handler
	 */
	*handle = (ENGINE_HANDLE*) engine;

	/* creo la cache de almacenamiento */


/*	void _cache_item_destroy(void *item){
		// La variable item es un elemento que esta
		// dentro del dictionary, el cual es un
		// t_dummy_ng_item. Este solo puede ser borrado
		// si no esta "storeado"

		((t_dummy_ng_item*)item)->stored = false;

		dummy_ng_item_release(NULL, NULL, item);
	}

	cache = dictionary_create(_cache_item_destroy);
*/
	return ENGINE_SUCCESS;
}

/*
 * Esta función se llama inmediatamente despues del create_instance y sirve para inicializar
 * la cache.
 */
static ENGINE_ERROR_CODE dummy_ng_initialize(ENGINE_HANDLE* handle, const char* config_str){
	/*
	 * En todas las funciones vamos a recibir un ENGINE_HANDLE* handle, pero en realidad
	 * el puntero handler es nuestra estructura t_dummy_ng
	 */
	t_dummy_ng *engine = (t_dummy_ng*)handle;


	/*
	 * En la variable config_str nos llega la configuración en el formato:
	 *
	 * 	cache_size=1024;chunk_size=48; etc ...
	 *
	 * La función parse_config recibe este string y una estructura que contiene
	 * las keys que debe saber interpretar, los tipos y los punteros de las variables donde tiene
	 * que almacenar la información.
	 *
	 */
	if (config_str != NULL) {
	  struct config_item items[] = {
		 { .key = "cache_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.cache_max_size },
		 { .key = "chunk_size",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.chunk_size },
		 { .key = "item_size_max",
		   .datatype = DT_SIZE,
		   .value.dt_size = &engine->config.block_size_max },
		 { .key = NULL}
	  };

	  double cuenta = 49*(engine->config.block_size_max/engine->config.chunk_size);
	  void *key_table= malloc(cuenta);

	  key_vector = key_table;

	  cache = malloc(engine->config.block_size_max);

	  //aca ya aloque lo del vector de keys y la cache.
	  void mtrace(void);

	  int lock = mlock (cache, engine->config.block_size_max);
	  if (lock == -1) {
	  		perror("Error locking the cache");
	  }

	  parse_config(config_str, items, NULL);

//		printf("%d", engine->config.block_size_max);
	}

	/*
	 * Registro la SIGUSR1. El registro de signals debe ser realizado en la función initialize
	 */
	signal(SIGUSR1, dummy_ng_dummp);

	return ENGINE_SUCCESS;
}


/*
 * Esta función es la que se llama cuando el engine es destruido
 */
static void dummy_ng_destroy(ENGINE_HANDLE* handle, const bool force){
	free(handle);
}

/*
 * Esto retorna algo de información la cual se muestra en la consola
 */
static const engine_info* dummy_ng_get_info(ENGINE_HANDLE* handle) {
	static engine_info info;
	return &info;
}

// Esta función es la que se encarga de allocar un item. Este item es la metadata necesaria para almacenar la key
// y el valor. Esta función solo se llama temporalemente antes de hacer, por ejemplo, un store. Luego del store
// el motor llama a la función release. Es por ello que utilizamos un flag "stored" para saber si el elemento
// allocado realmente fue almacenado en la cache o no.
// Puede ocurrir que este mismo se llame 2 veces para la misma operación. Esto es porque el protocolo ASCII de
// memcached hace el envio de la información en 2 partes, una con la key, size y metadata y otra parte con la data en si.
// Por lo que es posible que una vez se llame para hacer un allocamiento temporal del item y luego se llame otra vez, la cual
// si va a ser almacenada.
static ENGINE_ERROR_CODE dummy_ng_allocate(ENGINE_HANDLE *handler, const void* cookie, item **item, const void* key,
											const size_t nkey, const size_t nbytes, const int flags, const rel_time_t exptime){

	return ENGINE_SUCCESS;
}

/*
 * Destruye un item allocado, esta es una función que tiene que utilizar internamente memcached
 */
static void dummy_ng_item_release(ENGINE_HANDLE *handler, const void *cookie, item* item){
	free();
}

/*
 * Esta función lo que hace es mapear el item_info el cual es el tipo que memcached sabe manejar con el tipo de item
 * nuestro el cual es el que nosotros manejamos
 */
static bool dummy_ng_get_item_info(ENGINE_HANDLE *handler, const void *cookie, const item* item, item_info *item_info){
	// casteamos de item*, el cual es la forma generica en la cual memcached trata a nuestro tipo de item, al tipo
	// correspondiente que nosotros utilizamos

	return true;
}

/*
 * Esta funcion es invocada cuando memcached recibe el comando get
 */
static ENGINE_ERROR_CODE dummy_ng_get(ENGINE_HANDLE *handle, const void* cookie, item** item, const void* key, const int nkey, uint16_t vbucket){
	// transformamos  la variable key a string

	// buscamos y obtenemos el item

	//retornamos el item

	return ENGINE_SUCCESS;
}

/*
 * Esta función se llama cuando memcached recibe un set. La variable operation nos indica el tipo. Estos deben ser tratados indistintamente
 */
static ENGINE_ERROR_CODE dummy_ng_store(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t *cas, ENGINE_STORE_OPERATION operation, uint16_t vbucket){
	   return ENGINE_SUCCESS;
}

/*
 * Esta función se llama cuando memcached recibe un flush_all
 */
static ENGINE_ERROR_CODE dummy_ng_flush(ENGINE_HANDLE* handle, const void* cookie, time_t when) {

	//limpio toda la cache
	dictionary_clean(cache);

	return ENGINE_SUCCESS;
}

/*
 * Esta función se llama cuando memcached recibe un delete
 */
static ENGINE_ERROR_CODE dummy_ng_item_delete(ENGINE_HANDLE* handle, const void* cookie, const void* key, const size_t nkey, uint64_t cas, uint16_t vbucket) {
		return ENGINE_SUCCESS;
}

/*
 * ************************************* Funciones Dummy *************************************
 */

static ENGINE_ERROR_CODE dummy_ng_get_stats(ENGINE_HANDLE* handle, const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat) {
	return ENGINE_SUCCESS;
}

static void dummy_ng_reset_stats(ENGINE_HANDLE* handle, const void *cookie) {

}

static ENGINE_ERROR_CODE dummy_ng_unknown_command(ENGINE_HANDLE* handle, const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response) {
	return ENGINE_ENOTSUP;
}

static void dummy_ng_item_set_cas(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t val) {

}


/*
 * Handler de la SIGUSR1
 */
void dummy_ng_dummp(int signal){

	//ver que hacer con esta funcion
/*	void it(char *key, void *data){
		printf("%s\n", key);
	}

	dictionary_iterator(cache, it);
*/
}
