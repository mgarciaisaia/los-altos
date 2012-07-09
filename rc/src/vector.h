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

//busca la clave en el vector y devuelve la posicion en donde esta
int32_t vector_get(char *key);

//si es lru actualiza la hora del elemento, si no no.
void actualizar_key(key_element *it);
//si es buddy cuando elimina uno, ademas lo va compactando
uint32_t elimina_buddy(uint32_t posicion_org);

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *buscarLibreBuddy(size_t nbytes);
key_element *buscarLibreNext(size_t nbytes);
key_element *buscarLibreWorst(size_t nbytes);

void vector_clean(void);
void init_semaforos(void);
void destroy_semaforos(void);

uint32_t ordenar_vector(uint32_t posicion);
uint32_t eliminar_particion(int32_t valor);

#endif /* VECTOR_H_ */
