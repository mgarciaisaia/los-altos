/*
 * vector.h
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include "our_engine.h"

#ifndef VECTOR_H_
#define VECTOR_H_

extern key_element *key_vector;
//typedef key_vector key_vector;

//void inicializarVector(void);

void cargarEnVector(char *keys, void *data, size_t ndata, bool libre,
		uint32_t pos);

key_element *vector_get(char *key);

// Este no lo hago porque lo de crear el espacio lo hice en allocate en manage
//void *vector_create(void *_cache_item_destroy);

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *vector_search(void *cache, size_t nbytes);

//dictionary_put(cache, strkey, it);
void vector_put(char *key, key_element *item);

void vector_clean(void);

//void *item = dictionary_remove(cache, strkey);
void *vector_remove(char *key);

#endif /* VECTOR_H_ */
