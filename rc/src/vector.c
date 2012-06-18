/*
 * vector.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include <src/commons/config.h>
#include "vector.h"
#include "manage.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>

extern key_element *key_vector;
extern t_config* config;
extern uint32_t cantRegistros;
extern size_t part_minima;

int32_t ultima_posicion;
//static pthread_rwlock_t *posicion;
//static pthread_rwlock_t *keyVector;

void init_semaforos(void) {
	// pthread_rwlockattr_t * attr;
//	attr->__align = PTHREAD_PROCESS_SHARED;
//	attr->__size = sizeof(// pthread_rwlockattr_t);

// pthread_rwlockattr_init(attr);

	//	// pthread_rwlockattr_
	// pthread_rwlockattr_setpshared(attr, (int)PTHREAD_PROCESS_SHARED);

	// pthread_rwlock_init(posicion,attr);
	// pthread_rwlock_init(keyVector,attr);
}

void destroy_semaforos(void) {
	// pthread_rwlock_destroy(posicion);
	// pthread_rwlock_destroy(keyVector);
}

// fragmentacionI = espacio inutilizable
//uint32_t fragmentacionI;

int32_t buscarPosLibre(void) {
//analizar si aca conviene buscar a partir de donde quede para no recorrer todo siempre
	int32_t i = 0;

	// pthread_rwlock_rdlock(keyVector);

	while ((key_vector[i].data_size != 0) && (i < cantRegistros)) {
		i++;
	}

	if (i == cantRegistros)
		i = -1;

	// pthread_rwlock_unlock(keyVector);

	return i;

}

void cargarEnVector(char *keys, void *data, size_t ndata, bool libre,
		uint32_t pos) {
	// pthread_rwlock_wrlock(keyVector);

	key_vector[pos].key = keys;
	key_vector[pos].data = data;
	key_vector[pos].libre = libre;
	key_vector[pos].data_size = ndata;

	// pthread_rwlock_unlock(keyVector);
}

uint32_t borrar(void) {

	// pthread_rwlock_rdlock(keyVector);

	uint32_t resultado, i;

	struct timespec tp;
	tp = key_vector[0].tp;

	for (i = 1; i < cantRegistros; i++) {
		if (key_vector[i].tp.tv_sec < tp.tv_sec) {
			tp = key_vector[i].tp;
			resultado = i;
		} else {
			if (key_vector[i].tp.tv_sec == tp.tv_sec)
				if (key_vector[i].tp.tv_nsec < tp.tv_nsec) {
					tp = key_vector[i].tp;
					resultado = i;
				}
		}
	}

	// pthread_rwlock_unlock(keyVector);

	return resultado;
}

/*uint32_t borrarLRU(void) {
 uint32_t resultado = 1;
 return resultado;
 }*/

//pregunta el modo FIFO o LRU para guardarlo y asi la funcion borrar no discrimina el tipo lru o fifo
void actualizar_key(key_element *it) {
	struct timespec tp;

	// pthread_rwlock_wrlock(keyVector);

	char *string = config_get_string_value(config, "VICTIM");
	if (strcmp(string, "LRU") == 0) {
		clock_gettime(0, &tp);
		it->tp = tp;
	}
	// pthread_rwlock_unlock(keyVector);

}

// Me sirve para los 2 algoritmos
uint32_t eliminar_particion(void) {

	uint32_t resultado;

	resultado = borrar();

	// pthread_rwlock_wrlock(keyVector);

	key_vector[resultado].libre = true;
	key_vector[resultado].stored = false;

	// pthread_rwlock_unlock(keyVector);

	return resultado;
}

void compactarDinam(void) {

}

void compactarBuddy(void) {

}

void vector_inicializar(char *keys_space, void *cache, size_t cache_size) {
	uint32_t i;
//	fragmentacionI = 0;
	ultima_posicion = 0;
	cargarEnVector(keys_space, cache, cache_size, true, 0);

//	// pthread_rwlock_wrlock(keyVector);

	for (i = 1; i < cantRegistros; i++) {
		key_vector[i].libre = true;
		key_vector[i].stored = false;
		key_vector[i].data_size = 0;
	}
//	// pthread_rwlock_unlock(keyVector);

}

key_element *buscarLibreNext(size_t espacio) {

	key_element *resultado;
	int16_t busquedas_fallidas = 0;
//	// pthread_rwlock_wrlock(posicion);

	uint32_t i = ultima_posicion;
	char encontrado = 0;

	// calcula si va a haber fragmentacion
	int32_t resto = espacio % part_minima;
	int32_t diferencia = part_minima - resto;

	// pthread_rwlock_rdlock(keyVector);
// aca dentro hay funciones que usan el bloqueo de escritura,
// eso no me afecta?
	while (encontrado == 0) {

		while (!(key_vector[i].libre) && i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros) {
			i = 0;
		} else {
			if (key_vector[i].data_size >= espacio) {
				encontrado = 1;
			}
		}
		while ((encontrado != 1)
				&& (!(key_vector[i].libre) && i < ultima_posicion)) {
			i++;
		}
		if ((i == ultima_posicion) && (encontrado != 1)) {

			// si freq = -1 indica compactar cuando elimine todas
			int32_t frecuencia = config_get_int_value(config, "FREQ");
			busquedas_fallidas++;

			/* preguntar frecuencia de compactacion; si aplica,
			 * compactar, sino preguntar si es FIFO o LRU y borrar
			 * una FREQ*/

			if ((busquedas_fallidas < frecuencia) || (frecuencia = -1)) {

				//eliminar una particion segun esquema
				uint32_t particion = eliminar_particion();
				i = particion;
			} else {
				// compactar
				compactarDinam();
//				fragmentacionI = 0;
			}
		} else {
			if (key_vector[i].data_size >= espacio)

				encontrado = 1;
		}
	}
	// pthread_rwlock_unlock(keyVector);

//	// pthread_rwlock_wrlock(posicion);

	if ((i + 1) < cantRegistros)
		ultima_posicion = i + 1;
	else
		ultima_posicion = 0;

	// pthread_rwlock_unlock(posicion);

	/* Creo la particion sgte libre si corresponde */
	/* le digo a la sgt pos que tiene menos espacio del q realmente tiene, por efecto de la fragmentacion.
	 Al compactar tendria que preguntar de nuevo por el resto para tenerlo en cuenta,
	 porque ahora hago como q no existe
	 */
	//como ya sincronice las fn aca no pongo semaforos
	if ((key_vector[i].data_size - espacio - diferencia) > 0) {
		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			int32_t MAX_KEY = config_get_int_value(config, "MAX_KEY");
			cargarEnVector((key_vector[i].key + MAX_KEY),
					key_vector[i].data + espacio + diferencia,
					key_vector[i].data_size - espacio - diferencia, true,
					posicion);
		} else
			printf("No hay posiciones libres en el vector");
	}

	resultado = &key_vector[i];
	return resultado;

}

key_element *buscarLibreWorst(size_t espacio) {

	key_element *resultado;
	int16_t busquedas_fallidas = 0;
	uint32_t i = 0;
	uint32_t pos_mayor_tamano = 0;
	uint32_t mayor_tamano = 0;
	char encontrado = 0;

	// calcula si va a haber fragmentacion
	int32_t resto = espacio % part_minima;
	int32_t diferencia = part_minima - resto;

	// pthread_rwlock_rdlock(keyVector);

	while (((encontrado == 0) || (key_vector[i].data_size > 0))
			&& (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {

			if (mayor_tamano == 0) {

				//si llegue al final sin encontrar ninguno

				int32_t frecuencia = config_get_int_value(config, "FREQ");
				busquedas_fallidas++;

				/* preguntar frecuencia de compactacion; si aplica,
				 * compactar, sino preguntar si es FIFO o LRU y borrar
				 * una FREQ*/

				if ((busquedas_fallidas < frecuencia) || (frecuencia = -1)) {
					//eliminar una particion segun esquema
					i = eliminar_particion();
				} else {
					// compactar
					compactarDinam();
//				fragmentacionI = 0;
				}

				if (key_vector[i].data_size >= espacio) {
					encontrado = 1;
					pos_mayor_tamano = i;
					mayor_tamano = key_vector[i].data_size;
				} else
					encontrado = 0;
			} else {
				//si llegue al final pero habia encontrado algo
				encontrado = 1;
			}
		} else {
			if ((key_vector[i].data_size >= espacio)
					&& key_vector[i].data_size > mayor_tamano) {

				pos_mayor_tamano = i;
				mayor_tamano = key_vector[i].data_size;

			}

		}

		i++;

	}

	// pthread_rwlock_unlock(keyVector);

	/* Creo la particion sgte libre si corresponde */
	/* le digo a la sgt pos que tiene menos espacio del q realmente tiene, por efecto de la fragmentacion.
	 Al compactar tendria que preguntar de nuevo por el resto para tenerlo en cuenta,
	 porque ahora hago como q no existe
	 */
	if ((key_vector[i].data_size - espacio - diferencia) > 0) {
		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			int32_t MAX_KEY = config_get_int_value(config, "MAX_KEY");
			cargarEnVector((key_vector[pos_mayor_tamano].key + MAX_KEY),
					key_vector[pos_mayor_tamano].data + espacio + diferencia,
					key_vector[pos_mayor_tamano].data_size - espacio
							- diferencia, true, posicion);
		} else
			printf("No hay posiciones libres en el vector");
	}

	resultado = &key_vector[pos_mayor_tamano];
	return resultado;
}

//busca el item con esa key y lo devuelve.
key_element *vector_get(char *key) {
	key_element *resultado;
	uint32_t i;
	//buscar la key en el vector

	// pthread_rwlock_rdlock(keyVector);

	while ((strcmp(key_vector[i].key, key) != 0) && (i < cantRegistros)) {
		i++;
	}
	if (i == cantRegistros) {
		//llegue al final y no la encontre
		printf("No se encontró la clave");
		resultado = NULL;
	} else {
		if (!key_vector[i].libre)
			resultado = &(key_vector[i]);
		else
			resultado = NULL;
	}
	// pthread_rwlock_unlock(keyVector);

	return resultado;
}

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *buscarLibreBuddy(size_t espacio) {

	key_element *resultado;
	int16_t busquedas_fallidas = 0;
	uint32_t i = 0;
	char encontrado = 0;

	// calcula si va a haber fragmentacion
	int32_t resto = espacio % part_minima;
	int32_t diferencia = part_minima - resto;

	// pthread_rwlock_rdlock(keyVector);

	while (((encontrado == 0) || (key_vector[i].data_size > 0))
			&& (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {

			//si llegue al final sin encontrar ninguno

			int32_t frecuencia = config_get_int_value(config, "FREQ");
			busquedas_fallidas++;

			/* preguntar frecuencia de compactacion; si aplica,
			 * compactar, sino preguntar si es FIFO o LRU y borrar
			 * una FREQ*/

			if ((busquedas_fallidas < frecuencia) || (frecuencia = -1)) {
				//eliminar una particion segun esquema
				i = eliminar_particion();
			} else
				// compactar
				compactarBuddy();

			if (key_vector[i].data_size >= espacio)
				encontrado = 1;
			else
				encontrado = 0;

		} else {
			//si encontre algo

			/* ver de encontrar el mejor lugar y sino partir la memoria hasta encontrarlo*/
			encontrado = 1;
		}

		i++;
	}

	// pthread_rwlock_unlock(keyVector);

	/* Creo la particion sgte libre si corresponde */
	/* le digo a la sgt pos que tiene menos espacio del q realmente tiene, por efecto de la fragmentacion.
	 Al compactar tendria que preguntar de nuevo por el resto para tenerlo en cuenta,
	 porque ahora hago como q no existe
	 *
	 if ((key_vector[i].data_size - espacio - diferencia) > 0) {
	 int32_t posicion = buscarPosLibre();
	 if (posicion != -1) {
	 int32_t MAX_KEY = config_get_int_value(config, "MAX_KEY");
	 cargarEnVector((key_vector[pos_mayor_tamano].key + MAX_KEY),
	 key_vector[pos_mayor_tamano].data + espacio + diferencia,
	 key_vector[pos_mayor_tamano].data_size - espacio - diferencia, true, posicion);
	 } else
	 printf("No hay posiciones libres en el vector");
	 }

	 */
	resultado = &key_vector[i];
	return resultado;
}

