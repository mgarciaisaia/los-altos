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

//inicializa el vector
void vector_inicializar(char*, void*, size_t);

void cargarEnVector(char *keys, void *data, size_t ndata, bool libre,
		uint32_t pos);

//busca la clave en el vector
key_element *vector_get(char *key);

//si es lru actualiza la hora del elemento, si no no.
void actualizar_key(key_element *it);

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *vector_search(void *cache, size_t nbytes);

void vector_clean(void);

#endif /* VECTOR_H_ */
