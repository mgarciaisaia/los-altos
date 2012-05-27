/*
 * vector.h
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include "our_engine.h"

#ifndef VECTOR_H_
#define VECTOR_H_

key_element *key_vector;
//typedef key_vector key_vector;

//dictionary_get(cache, strkey);
key_element *vector_get(void *cache, char *key);

//cache = dictionary_create(_cache_item_destroy);
void *vector_create(void *_cache_item_destroy);

//dictionary_put(cache, strkey, it);
// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
void vector_put(key_element *cache, char *key, void *data);

//dictionary_clean(cache);
void vector_clean(key_element *cache);

//void *item = dictionary_remove(cache, strkey);
void *vector_remove(key_element *cache, char *key);

#endif /* VECTOR_H_ */
