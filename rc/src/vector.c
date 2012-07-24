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
#include <stdlib.h>
#include <src/commons/log.h>

extern t_log *logger;
extern key_element *key_vector;
extern t_config* config;
extern uint32_t cantRegistros;
extern size_t part_minima, cache_size;
extern char *keys_space;
extern void *cache;

int32_t ultima_posicion;
static pthread_rwlock_t *posicion;
static pthread_rwlock_t *keyVector;

void init_semaforos(void) {

	posicion = malloc(sizeof(pthread_rwlock_t));
	keyVector = malloc(sizeof(pthread_rwlock_t));
	pthread_rwlock_init(posicion, NULL);
	pthread_rwlock_init(keyVector, NULL);
}

void destroy_semaforos(void) {
	pthread_rwlock_destroy(posicion);
	pthread_rwlock_destroy(keyVector);
}

int32_t buscarPosLibre(void) {
//analizar si aca conviene buscar a partir de donde quede para no recorrer todoO siempre
	int32_t i = 0;

	pthread_rwlock_rdlock(keyVector);

	while ((key_vector[i].data_size != 0) && (i < cantRegistros)) {
		i++;
	}

	if (i == cantRegistros)
		i = -1;

	pthread_rwlock_unlock(keyVector);

	return i;

}

void cargarEnVector(char *keys, void *data, size_t ndata, bool libre,
		uint32_t pos) {
	pthread_rwlock_wrlock(keyVector);

	key_vector[pos].key = keys;
	key_vector[pos].data = data;
	key_vector[pos].libre = libre;
	key_vector[pos].data_size = ndata;

	pthread_rwlock_unlock(keyVector);
}

uint32_t borrar(void) {

	pthread_rwlock_rdlock(keyVector);

	uint32_t resultado = cantRegistros;
	uint32_t i = 0;
	struct timespec tp;

	while ((key_vector[i].libre) && i < cantRegistros) {
		i++;
	}
	tp = key_vector[i].tp;
	resultado = i;

	while ((key_vector[i].data_size > 0 && i < cantRegistros)
			&& (key_vector[i].tp.tv_sec != 0)) {

		if (key_vector[i].libre)
			i++;
		while ((i < cantRegistros && key_vector[i].tp.tv_sec != 0)
				&& !key_vector[i].libre) {
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
			i++;
		}
	}

	pthread_rwlock_unlock(keyVector);

	return resultado;
}

//pregunta el modo FIFO o LRU para guardarlo y asi la funcion borrar no discrimina el tipo lru o fifo
void actualizar_key(key_element *it) {
	struct timespec tp;

	char *string = config_get_string_value(config, "VICTIM");
	if (strcmp(string, "LRU") == 0) {

		pthread_rwlock_wrlock(keyVector);

		clock_gettime(0, &tp);
		it->tp = tp;

		pthread_rwlock_unlock(keyVector);
	}
}

// Me sirve para los 2 algoritmos
//devuelve la posicion borrada
uint32_t eliminar_particion(int32_t valor) {

	uint32_t resultado;

//si entra aca fue porque habia que borrar segun fifo o lru
	if (valor == -1){
	resultado = borrar();
	char *string = config_get_string_value(config, "VICTIM");
	log_debug(logger, "Se elimina la key: %s, usando el algoritmo %s", key_vector[resultado].key , string);
	}else
	resultado = valor;

	if (resultado != cantRegistros) {
//aca muere ¿porque? ya no hay semaforo de lectura.Solo para buddy no funciona
		pthread_rwlock_wrlock(keyVector);

		key_vector[resultado].data_size = key_vector[resultado].data_size
				+ key_vector[resultado].data_unuse;
		key_vector[resultado].libre = true;
		key_vector[resultado].stored = false;
		key_vector[resultado].data_unuse = 0;

		pthread_rwlock_unlock(keyVector);
	}
	//no habia mas nada para borrar y devuelvo el res

	return resultado;
}

uint32_t ordenar_vector(uint32_t posicion) {

	uint32_t i, j;

	pthread_rwlock_wrlock(keyVector);

	for (i = 0; i < cantRegistros; i++) {

		if (i == 8000)
			printf("valor %d \n",i);

		for (j = 0; j < cantRegistros; j++) {

			//&& ((key_vector[j].data_size > 0) && (key_vector[j+1].data_size > 0) ) sin eso me quedan todos los vacios adelante
			if ((key_vector[j].data > key_vector[j + 1].data)
					&& ((key_vector[j].data_size > 0)
							&& (key_vector[j + 1].data_size > 0))) {
				if (j == posicion)
					posicion = j + 1;
				key_element aux = key_vector[j];
				key_vector[j] = key_vector[j + 1];
				key_vector[j + 1] = aux;
			}
		}
	}

	pthread_rwlock_unlock(keyVector);

	return posicion;
}

uint32_t elimina_buddy(uint32_t posicion_org) {

	// cuando lo ordeno no pierdo la ref a mi dato?
	int32_t posicion_new = ordenar_vector(posicion_org);

	// pregunto si el siguiente o el anterior estan vacios, si es asi los junto
	int32_t i = 1;

	bool termine = false;

	pthread_rwlock_wrlock(keyVector);

	while (((((posicion_new) < cantRegistros) && (posicion_new) >= 0))
			&& !termine) {

		if (key_vector[posicion_new].libre){
		if ((((posicion_new + i) < cantRegistros)
				&& (key_vector[posicion_new + i].libre
						&& key_vector[posicion_new + i].data_size > 0)) && (key_vector[posicion_new + i].data_size == key_vector[posicion_new].data_size))  {

			// juntarlos y actualizar el que borre. tener en cuenta el espacio inutilizado. Crear el nuevo campo en el vector

			key_vector[posicion_new].data_size =
					key_vector[posicion_new].data_size
							+ (key_vector[posicion_new + i].data_size)
							+ key_vector[posicion_new + i].data_unuse;
			key_vector[posicion_new + i].data_size = 0;
		}
		if ((((posicion_new - i) > 0)
				&& (key_vector[posicion_new - i].libre
						&& key_vector[posicion_new - i].data_size > 0)) && (key_vector[posicion_new - i].data_size == key_vector[posicion_new].data_size))  {

			// juntarlos y actualizar el que borre. tener en cuenta el espacio inutilizado.

			key_vector[posicion_new].data_size =
					key_vector[posicion_new].data_size
							+ (key_vector[posicion_new - i].data_size
									+ key_vector[posicion_new - i].data_unuse);
			key_vector[posicion_new - i].data_size = 0;
		}
		}
		if (((!key_vector[posicion_new + i].libre)
				&& !(key_vector[posicion_new - i].libre))
				|| ((key_vector[posicion_new + i].data_size > 0)
						&& (key_vector[posicion_new - i].data_size > 0))
				|| (((posicion_new - i) < 0)
						|| ((posicion_new + i) == cantRegistros)))
			termine = true;

		i++;

	}
	pthread_rwlock_unlock(keyVector);

	return posicion_new;
}

uint32_t compactarDinam(void) {

	log_info(logger,"Se compacta la cache");

	// como ordenar recibe algo le mando cualquier posicion
	int32_t posicion_new = ordenar_vector(0);
	posicion_new = 1;
	//empiezo desde cero y me fijo si esta vacio el siguiente
	int32_t i = 0;

		bool termine = false;

	if ((key_vector[i].libre)
			&& key_vector[i].data_size == cache_size)
		termine =true;

	uint32_t ultimo_libre = i;

	pthread_rwlock_wrlock(keyVector);

	while ((i < cantRegistros) && !termine) {

		// pregunto si en mi posicion hay datos, si hay pregunto por la pos sgte
//			if (!key_vector[posicion_new].libre){

//listo : si esta libre que tmb entre y los junte!!!

		//al reves, pregunto si en la siguiente hay datos,y dsp si habia en la anterior y los muevo
		if (((posicion_new + i) < cantRegistros)
				&& (key_vector[posicion_new + i].libre)) {

			if ((key_vector[i].libre)
					&& key_vector[posicion_new + i].data_size > 0) {

				key_vector[i + posicion_new].data_size = key_vector[i
						+ posicion_new].data_size + key_vector[i].data_size;
				key_vector[i + posicion_new].data = key_vector[i].data;
				key_vector[i].data = NULL;
				key_vector[i].data_size = 0;

				ultimo_libre = i + posicion_new;
			}

		}

		//al reves, pregunto si en la siguiente hay datos,y dsp si habia en la anterior y los muevo
		if (((posicion_new + i) < cantRegistros)
				&& (!key_vector[posicion_new + i].libre)) {
			//	&& key_vector[posicion_new + i].data_size > 0)) {

			// si el sgte esta libre y y tiene espacio para datos compacto y dsp actualizo la referencia

			if (key_vector[i].libre) {

				//compacto
				memmove(key_vector[i].data, key_vector[posicion_new + i].data,
						key_vector[posicion_new + i].data_size);
				//actualizo el vector
				key_element aux;
				aux.key = key_vector[i].key;
				aux.data_size = key_vector[i].data_size;
				aux.data_unuse = key_vector[i].data_unuse;
				aux.data = key_vector[i].data
						+ key_vector[posicion_new + i].data_size;

				//aca no me actualizo el estado de ocupado
				key_vector[i] = key_vector[posicion_new + i];
				key_vector[posicion_new + i].data_size = aux.data_size;
				key_vector[posicion_new + i].data = aux.data;
				key_vector[posicion_new + i].data_unuse = aux.data_unuse;
				key_vector[posicion_new + i].key = aux.key;
				key_vector[posicion_new + i].stored = false;
				key_vector[posicion_new + i].libre = true;

				ultimo_libre = i + posicion_new;
			}
		} else {
			if ((((posicion_new + i) == cantRegistros) && (i < cantRegistros))
					&& key_vector[i].data_size > 0) {
				if ((key_vector[i].libre)
						&& key_vector[i - posicion_new].libre) {
					//al ultimo valor agregarle el espacio q quedo
					key_vector[i - posicion_new].data_size = key_vector[i
							- posicion_new].data_size + key_vector[i].data_size;
					key_vector[i].data_size = 0;

					ultimo_libre = i - posicion_new;

				}
			} else if ((posicion_new + i) == cantRegistros)
				termine = true;
		}
		i++;

	}
	pthread_rwlock_unlock(keyVector);

	return ultimo_libre;
}

void vector_inicializar(char *keys_space, void *cache, size_t cache_size) {
	uint32_t i;
	ultima_posicion = 0;
	cargarEnVector(keys_space, cache, cache_size, true, 0);

	int32_t MAX_KEY = config_get_int_value(config, "MAX_KEY");

	pthread_rwlock_wrlock(keyVector);

	for (i = 1; i < cantRegistros; i++) {
		key_vector[i].key = key_vector[i - 1].key + MAX_KEY;
		key_vector[i].libre = true;
		key_vector[i].stored = false;
		key_vector[i].data_size = 0;
		key_vector[i].data_unuse = 0;
	}
	pthread_rwlock_unlock(keyVector);

}

key_element *buscarLibreNext(size_t espacio) {

	key_element *resultado;
	int16_t busquedas_fallidas = 0;
	uint32_t particion;

	uint32_t i = ultima_posicion;
	char encontrado = 0;

	int32_t diferencia = 0;
// calcula si va a haber fragmentacion
	int32_t resto = espacio % part_minima;
	if (resto != 0)
		diferencia = part_minima - resto;

	pthread_rwlock_rdlock(keyVector);
// aca dentro hay funciones que usan el bloqueo de escritura,
// eso no me afecta? si.. porque se quedan esperando que termine
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
		//&& (!(key_vector[i].libre)
		while ((encontrado != 1) && (i < ultima_posicion)) {
			i++;
		}
		if ((i == ultima_posicion) && (encontrado != 1)) {

			// si freq = -1 indica compactar cuando elimine todas
			int32_t frecuencia = config_get_int_value(config, "FREQ");
			busquedas_fallidas++;

			/* preguntar frecuencia de compactacion; si aplica,
			 * compactar, sino preguntar si es FIFO o LRU y borrar
			 * una FREQ*/
			pthread_rwlock_unlock(keyVector);

			if ((busquedas_fallidas < frecuencia) || (frecuencia == -1)) {
				//eliminar una particion segun esquema
				particion = eliminar_particion(-1);

				if (particion == cantRegistros)
					vector_inicializar(keys_space, cache, cache_size);
				else
					i = particion;

			} else {
				// compactar
				i = compactarDinam();
				busquedas_fallidas = 0;
			}

			pthread_rwlock_rdlock(keyVector);

			if ((key_vector[i].data_size) >= espacio)
				encontrado = 1;

		} else {
			if (key_vector[i].data_size >= espacio)
				encontrado = 1;
			else
				i = 0;
		}
	}
	pthread_rwlock_unlock(keyVector);

	pthread_rwlock_wrlock(posicion);

	if ((i + 1) < cantRegistros)
		ultima_posicion = i + 1;
	else
		ultima_posicion = 0;

	pthread_rwlock_unlock(posicion);

	/* Creo la particion sgte libre si corresponde */
	/* le digo a la sgt pos que tiene menos espacio del q realmente tiene, por efecto de la fragmentacion.
	 Al compactar tendria que preguntar de nuevo por el resto para tenerlo en cuenta,
	 porque ahora hago como q no existe
	 */
//como ya sincronice las fn aca no pongo semaforos, eso no pincharia?
	if ((key_vector[i].data_size - espacio - diferencia) > 0) {
		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			cargarEnVector((key_vector[posicion].key),
					key_vector[i].data + espacio + diferencia,
					key_vector[i].data_size - espacio - diferencia, true,
					posicion);

			key_vector[i].data_unuse = diferencia;
		} else{
			log_error(logger, "La memoria cache se encuentra llena");
			//printf("No hay posiciones libres en el vector");
//			return ENGINE_FAILED;
	}
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
	uint32_t particion = 0;
	int32_t diferencia = 0;
// calcula si va a haber fragmentacion
	int32_t resto = espacio % part_minima;
	if (resto != 0)
		diferencia = part_minima - resto;

	pthread_rwlock_rdlock(keyVector);

	while (((encontrado == 0) || (key_vector[i].data_size > 0))
			&& (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {

			if (mayor_tamano == 0) {

				//si llegue al final sin encontrar ninguno
				pthread_rwlock_unlock(keyVector);

				int32_t frecuencia = config_get_int_value(config, "FREQ");
				busquedas_fallidas++;

				/* preguntar frecuencia de compactacion; si aplica,
				 * compactar, sino preguntar si es FIFO o LRU y borrar
				 * una FREQ*/

				if ((busquedas_fallidas < frecuencia) || (frecuencia == -1)) {
					//eliminar una particion segun esquema
					particion = eliminar_particion(-1);

					if (particion == cantRegistros)
						vector_inicializar(keys_space, cache, cache_size);
					else
						i = particion;

				} else {
					// compactar
					i = compactarDinam();
					busquedas_fallidas = 0;
				}

				pthread_rwlock_rdlock(keyVector);

				if ((key_vector[i].data_size) >= espacio) {
					pos_mayor_tamano = i;
					mayor_tamano = key_vector[i].data_size;
					encontrado = 1;

				} else {
					/*	if (particion == cantRegistros)
					 compactarDinam();
					 else
					 */i = 0;
				}
			} else {
				//si llegue al final pero habia encontrado algo
				encontrado = 1;
			}
		} else {
			if (((key_vector[i].data_size >= espacio)
					&& key_vector[i].data_size > mayor_tamano)
					&& (key_vector[i].libre)) {

				pos_mayor_tamano = i;
				mayor_tamano = key_vector[i].data_size;

			}

		}

		i++;

	}

	pthread_rwlock_unlock(keyVector);

	/* Creo la particion sgte libre si corresponde */
	/* le digo a la sgt pos que tiene menos espacio del q realmente tiene, por efecto de la fragmentacion.
	 Al compactar tendria que preguntar de nuevo por el resto para tenerlo en cuenta,
	 porque ahora hago como q no existe
	 */
	if ((key_vector[i].data_size - espacio - diferencia) > 0) {
		int32_t posicionn = buscarPosLibre();
		if (posicionn != -1) {
			cargarEnVector((key_vector[posicionn].key),
					key_vector[pos_mayor_tamano].data + espacio + diferencia,
					key_vector[pos_mayor_tamano].data_size - espacio
							- diferencia, true, posicionn);
			key_vector[pos_mayor_tamano].data_unuse = diferencia;
		} else{
			log_error(logger, "La memoria cache se encuentra llena");
			//printf("No hay posiciones libres en el vector");
//			return ENGINE_FAILED;
		}
	}
	resultado = &key_vector[pos_mayor_tamano];
	return resultado;
}


// aca adentro buscar separando los algoritmos next y worst y cuando compacte que separe buddy y dinamica
key_element *buscarLibreBuddy(size_t espacio) {

	key_element *resultado;
	uint32_t i = 0;
	char encontrado = 0;
	uint32_t lugares = cantRegistros;

	pthread_rwlock_rdlock(keyVector);
//|| (key_vector[i].data_size > 0)
	while (((encontrado == 0)) && (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {

			if (((lugares == 0) && (key_vector[lugares].data_size < espacio))
					|| !key_vector[lugares].libre) {

				// entra aca si no encontro espacio

					pthread_rwlock_unlock(keyVector);

				i = eliminar_particion(-1);

				//Aca me devuelve la nueva posicion dsp de ordenado junto con el nuevo tamaño dsp de compactar en caso de haberlo comprimido
				uint32_t new_pos = elimina_buddy(i);
				i = new_pos;
					pthread_rwlock_rdlock(keyVector);

				// pregunto si en el nuevo lugar entra
				if ((key_vector[i].data_size > espacio)
						&& (key_vector[i].libre)) {
					//	&& (key_vector[i].data_size <= key_vector[lugares].data_size)
					lugares = i;
					encontrado = 1;
				}
			} else {
				//si llegue al final pero habia encontrado algo
				encontrado = 1;
			}
		} else {

			if (((encontrado == 0) &&(key_vector[i].data_size >= espacio))
					&& ((key_vector[i].libre))){
				if (lugares == cantRegistros)
				lugares = i;
				else{
					if (key_vector[i].data_size <= key_vector[lugares].data_size)
						lugares = i;
				}

			}
		}
		i++;
	}

	uint32_t prox_espacio;

	bool verdad = false;
	do {

		uint32_t prox_particion = key_vector[lugares].data_size / 2;

		if ((espacio <= prox_particion) && (prox_particion >= part_minima)) {
			//CREAR LA PARTICION SIGUIENTE
			prox_espacio = key_vector[lugares].data_size / 2;

			// actualizar las particiones
			int32_t posicionn = buscarPosLibre();

				pthread_rwlock_unlock(keyVector);

			if (posicionn != -1) {

				cargarEnVector((key_vector[posicionn].key),
						key_vector[lugares].data + prox_espacio, prox_espacio,
						true, posicionn);
			} else
				printf("No hay posiciones libres en el vector");

				pthread_rwlock_wrlock(keyVector);
			key_vector[lugares].data_size = prox_espacio;
			prox_particion = prox_espacio;
				pthread_rwlock_unlock(keyVector);
				pthread_rwlock_rdlock(keyVector);

		} else {
			/*ya no lo puedo partir mas o mi espacio es mayor a la particion que cree.*/
			pthread_rwlock_unlock(keyVector);
//					if(pthread_rwlock_trywrlock(keyVector))
//						perror("trylock");
//					else
//						printf("Entro try lock\n");

			pthread_rwlock_wrlock(keyVector);
			if (espacio > prox_particion)
				key_vector[lugares].data_unuse = key_vector[lugares].data_size
						- espacio;
			else
				key_vector[lugares].data_unuse = prox_particion - espacio;

				pthread_rwlock_unlock(keyVector);
				pthread_rwlock_rdlock(keyVector);

			resultado = &key_vector[lugares];

			verdad = true;
		}
	} while (!verdad);

	pthread_rwlock_unlock(keyVector);

	return resultado;
}

//busca el item con esa key y lo devuelve.
int32_t vector_get(char *key) {
//	key_element *resultado;
	int32_t resultado;
	uint32_t encontrado = 0;
	uint32_t i = 0;
//buscar la key en el vector
	size_t nkey = strlen(key);
	pthread_rwlock_rdlock(keyVector);

	while ((encontrado == 0) && (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {

//			while ((key_vector[i].libre) && (i < cantRegistros)) {
//				i++;
//			}
//			if (i == cantRegistros) {
			//llegue al final y no la encontre

			resultado = -1;
			encontrado = 1;
		} else {
			if ((!(key_vector[i].libre) && key_vector[i].nkey == nkey)
					&& (strncmp(key_vector[i].key, key, nkey) == 0)) {
				resultado = i;
				encontrado = 1;
			} else
				i++;
		}

	}
	pthread_rwlock_unlock(keyVector);

	return resultado;
}

