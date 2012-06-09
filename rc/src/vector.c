/*
 * vector.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include <src/commons/config.h>
#include "vector.h"
#include "our_engine.h"
#include <string.h>

#define MAX_KEY 41

extern key_element *key_vector;
extern int32_t ultima_posicion;
extern t_config* config;
extern uint32_t cantRegistros;

int32_t buscarPosLibre(void) {

	int32_t i = 0;

	while ((key_vector[i].data != NULL) && (i < cantRegistros)) {
		i++;
	}

	if (i == cantRegistros)
		i = -1;

	return i;

}

void cargarEnVector(char *keys, void *data, size_t ndata, bool libre,
		uint32_t pos) {

	key_vector[pos].key = keys;
	key_vector[pos].data = data;
	key_vector[pos].libre = libre;
	key_vector[pos].data_size = ndata;

}

uint32_t borrarFIFO(void) {
	uint32_t resultado = 1;

	//como se en que momento llegó?

	return resultado;
}

uint32_t borrarLRU(void) {
	uint32_t resultado = 1;
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

uint32_t buscarLibreNext(void *cache, size_t espacio) {

	/*ver que hacer cuando se juntan varios pedacitos menores a
	 * la particion minima. tendria que venir como parametro quiza
	 * con la diferencia ¿? o no .... VER que hacer con eso
	 */

	int16_t busquedas_fallidas = 0;
	uint32_t i = ultima_posicion;
	char encontrado = 0;

	while (encontrado == 0) {

		while (!(key_vector[i].libre) && i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros)
			i = 0;
		else {
			if (key_vector[i].data_size >= espacio)
				encontrado = 1;
		}
		while (!(key_vector[i].libre) && i < ultima_posicion) {
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
				i = particion;
			} else
				// compactar
				compactarDinam();

		} else {
			if (key_vector[i].data_size >= espacio)
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
		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			cargarEnVector((key_vector[i].key + MAX_KEY),
					key_vector[i].data + espacio,
					key_vector[i].data_size - espacio, false, posicion);
		} else
			printf("No hay posiciones libres en el vector");
	}

	return i;
}

uint32_t buscarLibreWorst(size_t espacio) {

	int16_t busquedas_fallidas = 0;
	uint32_t i = 0;
	uint32_t pos_mayor_tamano = 0;
	uint32_t mayor_tamano = 0;
	char encontrado = 0;

	while (encontrado == 0) {

		while (!(key_vector[i].libre) && i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros) {

			if (mayor_tamano == 0) {

				//si llegue al final sin encontrar ninguno

				int32_t frecuencia = config_get_int_value(config, "FREQ");
				busquedas_fallidas++;

				/* preguntar frecuencia de compactacion; si aplica,
				 * compactar, sino preguntar si es FIFO o LRU y borrar
				 * una FREQ*/

				if (busquedas_fallidas < frecuencia)
					//eliminar una particion segun esquema
					i = eliminar_particion();

				else
					// compactar
					compactarDinam();

				if (key_vector[i].data_size >= espacio) {
					encontrado = 1;
					pos_mayor_tamano = i;
					mayor_tamano = key_vector[i].data_size;
				} else
					encontrado = 0;
			} else {
				encontrado = 1;
//				uint32_t pos_mayor_tamano = i;
//				uint32_t mayor_tamano = key_vector[i].data_size;
			}
		} else {
			if ((key_vector[i].data_size >= espacio)
					&& key_vector[i].data_size > mayor_tamano) {

				pos_mayor_tamano = i;
				mayor_tamano = key_vector[i].data_size;

			}

		}
	}

	/* Creo la particion sgte libre */

	// PREGUNTAR SI EL DESPLAZAMIENTO RESULTA COMO YO QUIERO AL SER VOID
	if ((key_vector[i].data_size - espacio) > 0) {

		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			cargarEnVector((key_vector[i].key + MAX_KEY),
					key_vector[i].data + espacio,
					key_vector[i].data_size - espacio, false, posicion);
		} else
			printf("No hay posiciones libres en el vector");
	}
	return pos_mayor_tamano;

}

void *buscarElemento(key_element *key_vector, char *key) {

	return key_vector;
}

//dictionary_get(cache, strkey);
//busca el item con esa key y lo devuelve.
key_element *vector_get(void *cache, char *key) {
	key_element *resultado;

	//buscar la key en el vector

	return resultado;
}

// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *vector_search(void *cache, size_t nbytes) {

	key_element *resultado;

	uint32_t valor;
	char *string = config_get_string_value(config, "PARTICION");
	if (strcmp(string, "NEXT") == 0)
		valor = buscarLibreNext(cache, nbytes);
	else {
		if (strcmp(string, "WORST") == 0)
			valor = buscarLibreWorst(nbytes);
	}

	resultado = &key_vector[valor];
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
