/*
 * vector.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include <src/commons/config.h>
#include "vector.h"
#include "manage.h"

extern key_element *key_vector;
//extern key_element *last_posicion;
extern int32_t ultima_posicion;
extern t_config* config;
extern uint32_t cantRegistros;

uint32_t borrarFIFO(void) {
	uint32_t resultado;

	//como se en que momento llegó?

	return resultado;
}

uint32_t borrarLRU(void) {
	uint32_t resultado;
	return resultado;
}

// Me sirve para los 2 algoritmos
uint32_t eliminar_particion(void) {

	uint32_t resultado;
	char *string = config_get_string_value(config, "VICTIM");
	if (strcmp(string, "FIFO") == 0)
		resultado = borrarFIFO();
	else {
		if (strcmp(string, "LRU") == 0)
			resultado = borrarLRU();
	}

	return resultado;
}

void compactarDinam(void) {

}

key_element *buscarLibreNext(void *cache, size_t espacio) {

	/*ver que hacer cuando se juntan varios pedacitos menores a
	 * la particion minima. tendria que venir como parametro quiza
	 * con la diferencia ¿? o no .... VER que hacer con eso
	 */

	int16_t busquedas_fallidas = 0;
	key_element *resultado;
	uint32_t i = ultima_posicion;
	char encontrado = 0;

	while (encontrado == 0) {

		while (not(key_vector[i].libre) && i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros)
			i = 0;
		else {
			if (key_vector[i]->data_size >= espacio)
				encontrado = 1;
		}
		while (not(key_vector[i].libre) && i < ultima_posicion) {
			i++;
		}
		if (i == ultima_posicion) {

			int32_t frecuencia = config_get_int_value(config, "FREQ");
			busquedas_fallidas++;

			/* preguntar frecuencia de compactacion; si aplica,
			 * compactar, sino preguntar si es FIFO o LRU y borrar
			 * una FREQ*/

			if (busquedas_fallidas < frecuencia) {

				//eliminar una particion segun esquema
				uint32_t particion = eliminar_particion();

			} else
				// compactar
				compactarDinam();

		} else {
			if (key_vector[i]->data_size >= espacio)
				encontrado = 1;
		}
	}

	if ((i + 1) < cantRegistros)
		ultima_posicion = i + 1;
	else
		ultima_posicion = 0;

	/* Creo la particion sgte libre */

	// PREGUNTAR SI EL DESPLAZAMIENTO RESULTA COMO YO QUIERO AL SER VOID
	if ((key_vector[i].data_size - espacio) > 0) {
		key_vector[ultima_posicion].key = key_vector[i].key + 41;
		key_vector[ultima_posicion].data = key_vector[i].data + espacio;
		key_vector[ultima_posicion].data_size = key_vector[i].data_size
				- espacio;
		key_vector[ultima_posicion].libre = true;
	} else {
		key_vector[ultima_posicion].key = key_vector[i].key;
		key_vector[ultima_posicion].data = key_vector[i].data;
		key_vector[ultima_posicion].data_size = 0;
		key_vector[ultima_posicion].libre = true;
	}

	resultado = key_vector[i];
	return resultado;
}

key_element *buscarLibreWorst(size_t espacio) {

	int16_t busquedas_fallidas = 0;
	key_element *resultado;
	uint32_t i = 0;
	uint32_t pos_mayor_tamaño = 0;
	uint32_t mayor_tamaño = 0;
	char encontrado = 0;

	while (encontrado == 0) {

		while (not(key_vector[i].libre) && i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros) {

			if (mayor_tamaño == 0) {

				//si llegue al final sin encontrar ninguno

				int32_t frecuencia = config_get_int_value(config, "FREQ");
				busquedas_fallidas++;

				/* preguntar frecuencia de compactacion; si aplica,
				 * compactar, sino preguntar si es FIFO o LRU y borrar
				 * una FREQ*/

				if (busquedas_fallidas < frecuencia)
					//eliminar una particion segun esquema
					uint32_t particion = eliminar_particion();
				else
					// compactar
					compactarDinam();

				if (key_vector[i]->data_size >= espacio) {
					encontrado = 1;
					uint32_t pos_mayor_tamaño = i;
					uint32_t mayor_tamaño = key_vector[i]->data_size;
				} else
					encontrado = 0;
			} else {
				encontrado = 1;
				uint32_t pos_mayor_tamaño = i;
				uint32_t mayor_tamaño = key_vector[i]->data_size;
			}
		} else {
			if (key_vector[i]->data_size >= espacio)
				uint32_t pos_mayor_tamaño = i;
			uint32_t mayor_tamaño = key_vector[i]->data_size;
		}
	}

	/* Creo la particion sgte libre */

	// PREGUNTAR SI EL DESPLAZAMIENTO RESULTA COMO YO QUIERO AL SER VOID
//para actualizar el nuevo espacio tendria q buscar una entrada libre en el vector
	/*	if ((mayor_tamaño - espacio) > 0) {
	 key_vector[ultima_posicion].key = key_vector[i].key + 41;
	 key_vector[ultima_posicion].data = key_vector[i].data + espacio;
	 key_vector[ultima_posicion].data_size = key_vector[i].data_size
	 - espacio;
	 key_vector[ultima_posicion].libre = true;
	 } else {
	 key_vector[ultima_posicion].key = key_vector[i].key;
	 key_vector[ultima_posicion].data = key_vector[i].data;
	 key_vector[ultima_posicion].data_size = 0;
	 key_vector[ultima_posicion].libre = true;
	 }
	 */
	resultado = key_vector[pos_mayor_tamaño];

	return resultado;
}

void *buscarElemento(key_element *key_vector, char *key) {

	return key_vector;
}

//dictionary_get(cache, strkey);
key_element *vector_get(void *cache, char *key) {

	return key_vector;
}

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *vector_search(void *cache, size_t nbytes) {

	key_element * resultado;

	char *string = config_get_string_value(config, "PARTICION");
	if (strcmp(string, "NEXT") == 0)
		resultado = buscarLibreNext(cache, nbytes);
	else {
		if (strcmp(string, "WORST") == 0)
			resultado = buscarLibreWorst(nbytes);
	}

	return resultado;
}

//dictionary_put(cache, strkey, it);
void vector_put(key_element *cache, char *key, void *data) {

}

//dictionary_clean(cache);
void vector_clean(key_element *cache) {

}

//void *item = dictionary_remove(cache, strkey);
void *vector_remove(key_element *cache, char *key) {

	return key_vector;
}
