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

	uint32_t resultado =cantRegistros;

//si entra aca fue porque habia que borrar segun fifo o lru
	if (valor == -1) {
		resultado = borrar();
		char *string = config_get_string_value(config, "VICTIM");
		log_debug(logger, "Se elimina la key: %s, usando el algoritmo %s",
				key_vector[resultado].key, string);
	} else
		resultado = valor;

	if (resultado != cantRegistros) {
//aca muere ¿porque? ya no hay semaforo de lectura.Solo para buddy no funciona
		pthread_rwlock_wrlock(keyVector);

		key_vector[resultado].data_size += key_vector[resultado].data_unuse;
		key_vector[resultado].libre = true;
		key_vector[resultado].stored = false;
		key_vector[resultado].data_unuse = 0;

		pthread_rwlock_unlock(keyVector);
	}
	//no habia mas nada para borrar y devuelvo el res

	return resultado;
}

uint32_t ordenar_vector(int32_t posicion) {

	char * clave_original = NULL;
	uint32_t i = 0, j = 0, posicion2 = 0;

	uint32_t hasta = cantRegistros;
	if (posicion != -1) {
		clave_original = key_vector[posicion].key;
		posicion2 = posicion;
	}

	bool flag = false;
	pthread_rwlock_wrlock(keyVector);

	for (i = 1; i < (hasta - 1); i++) {
		flag = false;
		for (j = 0; j < hasta; j++) {

			if ((key_vector[j].data > key_vector[j + 1].data)
					&& ((key_vector[j].data_size > 0)
							&& (key_vector[j + 1].data_size > 0))) {
				flag = true;
				if (posicion != -1) {
					if (strcmp(key_vector[j].key, clave_original) == 0)
						posicion2 = j + 1;
				}
				key_element aux = key_vector[j];
				key_vector[j] = key_vector[j + 1];
				key_vector[j + 1] = aux;
			}
		}
		if (!flag)
			break;
	}
	pthread_rwlock_unlock(keyVector);

	return posicion2;
}

void juntarlos(int32_t actual, int32_t posicion) {

	// juntarlos y actualizar el que borre
	char cuenta = 1;

	pthread_rwlock_wrlock(keyVector);

//	int32_t padre = key_vector[actual].flags / 10;
	bool esPar = key_vector[actual].flags.padre % 2;
	if (esPar)
		cuenta = 2;

	key_vector[actual].flags.actual = key_vector[actual].flags.padre;

	if (key_vector[actual].flags.padre > 0)
		key_vector[actual].flags.padre -= cuenta;
	else
		key_vector[actual].flags.padre += cuenta;

	key_vector[actual].data_size += key_vector[posicion].data_size;
	key_vector[posicion].data_size = 0;

	pthread_rwlock_unlock(keyVector);

}

bool cumple(int32_t actual, int32_t posicionn) {

	bool rangoValido, sizeUtil = false, mismoPadre = false, estaLibre = false;

	rangoValido = (posicionn >= 0) && (posicionn < cantRegistros);

	if (rangoValido) {

		sizeUtil = (key_vector[posicionn].data_size > 0)
				&& (key_vector[posicionn].data_size
						== key_vector[actual].data_size);
		mismoPadre = key_vector[posicionn].flags.padre
				== key_vector[actual].flags.padre;
		estaLibre = key_vector[posicionn].libre;
	}

	return (rangoValido && sizeUtil && mismoPadre && estaLibre);
}

uint32_t elimina_buddy(uint32_t posicion_org) {

	uint32_t s = 0, a = 0;
	int32_t posicion = ordenar_vector(posicion_org);
	int32_t siguiente = posicion + 1;
	int32_t anterior = posicion - 1;

	bool termine = false;

	while (((posicion < cantRegistros) && (posicion >= 0)) && !termine) {

		if (cumple(posicion, siguiente)) {
			juntarlos(posicion, siguiente);
			s++;
		}
		if (cumple(posicion, anterior)) {
			juntarlos(posicion, anterior);
			a++;
		}

		siguiente += s;
		anterior -= a;

		if (!(cumple(posicion, siguiente)) && !(cumple(posicion, anterior)))
			termine = true;
	}

	return posicion;

}

int32_t compactarDinam(void) {

	log_info(logger, "Se compacta la cache");

	// como ordenar recibe algo le mando cualquier posicion
	int32_t posicion_new = ordenar_vector(-1);
	posicion_new = 1;
	//empiezo desde cero y me fijo si esta vacio el siguiente
	int32_t i = 0;
	bool compacte = false;
	bool termine = false;

	if ((key_vector[i].libre) && key_vector[i].data_size == cache_size)
		termine = true;

	int32_t ultimo_libre = i;

	pthread_rwlock_wrlock(keyVector);

	while ((i < cantRegistros) && !termine) {

		// pregunto si en mi posicion hay datos, si hay pregunto por la pos sgte
//			if (!key_vector[posicion_new].libre){

//listo : si esta libre que tmb entre y los junte!!!

//al reves, pregunto si en la siguiente hay datos,y dsp si habia en la anterior y los muevo
		if (((posicion_new + i) < cantRegistros)
				&& (key_vector[posicion_new + i].libre)) {

			if ((key_vector[i].libre && key_vector[i].data_size > 0)
					&& key_vector[posicion_new + i].data_size > 0) {

				compacte = true;
				key_vector[i + posicion_new].data_size +=
						key_vector[i].data_size;
				key_vector[i + posicion_new].data = key_vector[i].data;
				key_vector[i].data = NULL;
				key_vector[i].data_size = 0;

				ultimo_libre = i + posicion_new;
			}
		}

		//al reves, pregunto si en la siguiente hay datos,y dsp si habia en la anterior y los muevo
		if (((posicion_new + i) < cantRegistros)
				&& (!key_vector[posicion_new + i].libre)) {

			// si el sgte esta libre y y tiene espacio para datos compacto y dsp actualizo la referencia

			if (key_vector[i].libre && key_vector[i].data_size > 0) {
				compacte = true;
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
				if ((key_vector[i].libre && key_vector[i].data_size > 0)
						&& key_vector[i - posicion_new].libre) {

					compacte = true;
					//al ultimo valor agregarle el espacio q quedo
					key_vector[i - posicion_new].data_size +=
							key_vector[i].data_size;
					key_vector[i].data_size = 0;

					ultimo_libre = i - posicion_new;

				}
			} else if ((posicion_new + i) == cantRegistros)
				termine = true;
		}
		i++;

	}
	pthread_rwlock_unlock(keyVector);

	if (!compacte)
		return -1;
	return ultimo_libre;
}

void vector_inicializar(char *keys_space, void *cache, size_t cache_size) {
	uint32_t i;
	ultima_posicion = 0;
	cargarEnVector(keys_space, cache, cache_size, true, 0);

	int8_t MAX_KEY = config_get_int_value(config, "MAX_KEY");

	pthread_rwlock_wrlock(keyVector);
	key_vector[0].flags.actual = 0;
	key_vector[0].flags.padre = 0;
	for (i = 1; i < cantRegistros; i++) {
		key_vector[i].key = key_vector[i - 1].key + MAX_KEY;
		key_vector[i].libre = true;
		key_vector[i].stored = false;
		key_vector[i].data_size = 0;
		key_vector[i].data_unuse = 0;
		key_vector[i].flags.actual = key_vector[i].flags.padre = 0;
	}
	pthread_rwlock_unlock(keyVector);

}

key_element *buscarLibreNext(size_t espacio) {

	key_element *resultado = NULL;
	int16_t busquedas_fallidas = 0;
	uint32_t particion=0;

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
		while ((key_vector[i].libre && key_vector[i].data_size == 0)
				&& i < cantRegistros) {
			i++;
		}
		if (i == cantRegistros) {
			i = 0;
		} else {
			if (key_vector[i].data_size >= espacio) {
				encontrado = 1;
			}
		}

		while ((encontrado != 1) && (i < ultima_posicion)) {
			i++;
		}
		if ((i == ultima_posicion) && (encontrado != 1)) {

			// si freq = -1 indica compactar cuando elimine todas
			int32_t frecuencia = config_get_int_value(config, "FREQ");
			busquedas_fallidas++;

			/* preguntar frecuencia de compactacion; si aplica,
			 * compactar, sino borrar una FREQ*/
			pthread_rwlock_unlock(keyVector);

			if ((busquedas_fallidas < frecuencia) || (frecuencia == -1)) {

				particion = eliminar_particion(-1);

				if (particion == cantRegistros)
					vector_inicializar(keys_space, cache, cache_size);
				else
					i = particion;

			} else {
				// compactar
				int32_t compac = compactarDinam();
				if (compac == -1) {
					log_debug(logger,
							"No había datos que compactar. Se elimina una particion");
					i = eliminar_particion(-1);
				}else
					i = compac;
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
	if ((key_vector[i].data_size - espacio - diferencia) > 0) {
		int32_t posicion = buscarPosLibre();
		if (posicion != -1) {
			cargarEnVector((key_vector[posicion].key),
					key_vector[i].data + espacio + diferencia,
					key_vector[i].data_size - espacio - diferencia, true,
					posicion);

			key_vector[i].data_unuse = diferencia;
		} else {
			return resultado = NULL;
		//	log_error(logger, "No es posible crear mas particiones");
			uint32_t aux = eliminar_particion(-1);
			cargarEnVector((key_vector[posicion].key),
					key_vector[aux].data + espacio + diferencia,
					key_vector[aux].data_size - espacio - diferencia, true,
					posicion);

			key_vector[aux].data_unuse = diferencia;
			//printf("No hay posiciones libres en el vector");
//			return ENGINE_FAILED;
		}
	}

	resultado = &key_vector[i];
	return resultado;

}

key_element *buscarLibreWorst(size_t espacio) {

	key_element *resultado=NULL;
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
				 * compactar, sino borrar una FREQ*/

				if ((busquedas_fallidas = frecuencia) || (frecuencia == -1)) {

					particion = eliminar_particion(-1);

					if (particion == cantRegistros)
						vector_inicializar(keys_space, cache, cache_size);
					else
						i = particion;

				} else {
					// compactar
					int32_t compac = compactarDinam();
					if (compac == -1) {
						log_debug(logger,
								"No había datos que compactar. Se elimina una particion");
						i = eliminar_particion(-1);
					}else
						i = compac;

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
		} else {
			return resultado = NULL;
			//	log_error(logger, "No es posible crear mas particiones");
				uint32_t aux = eliminar_particion(-1);
				cargarEnVector((key_vector[aux].key),
						key_vector[aux].data + espacio + diferencia,
						key_vector[aux].data_size - espacio - diferencia, true,
						aux);

				key_vector[aux].data_unuse = diferencia;			//printf("No hay posiciones libres en el vector");
//			return ENGINE_FAILED;
		}
	}
	resultado = &key_vector[pos_mayor_tamano];
	return resultado;
}

void insert_PadreActual(int32_t posicion1, int32_t actual1, int32_t padre1,
		int32_t posicion2, int32_t actual2, int32_t padre2) {

	pthread_rwlock_wrlock(keyVector);

	key_vector[posicion1].flags.padre = padre1;
	key_vector[posicion1].flags.actual = actual1;
	key_vector[posicion2].flags.padre = padre2;
	key_vector[posicion2].flags.actual = actual2;

	pthread_rwlock_unlock(keyVector);

}

void actualizar_flags(uint32_t lugares, int32_t posicionn) {

	int32_t padre = key_vector[lugares].flags.padre;
	int32_t actual = key_vector[lugares].flags.actual;

	if (actual == 0) {
		//si es la primer division, al padre lo dejo en 0

		insert_PadreActual(posicionn, padre - 1, 0, lugares, padre + 1, 0);
	} else {
		//si no es la primer division

		if (actual < 0) {
			insert_PadreActual(posicionn, actual - 1, actual, lugares,
					actual - 2, actual);
		} else {
			insert_PadreActual(posicionn, actual + 1, actual, lugares,
					actual + 2, actual);
		}
	}

}

key_element * crear_arbol(uint32_t lugares, size_t espacio) {

	key_element *resultado = NULL;
	uint32_t prox_espacio = 0;
	bool verdad = false;

	do {
		pthread_rwlock_rdlock(keyVector);
		uint32_t prox_particion = key_vector[lugares].data_size / 2;
		pthread_rwlock_unlock(keyVector);

		if ((espacio <= prox_particion) && (prox_particion >= part_minima)) {
			//CREAR LA PARTICION SIGUIENTE
			prox_espacio = prox_particion;

//			pthread_rwlock_unlock(keyVector);
			int32_t posicionn = buscarPosLibre();

			if (posicionn != -1) {

				actualizar_flags(lugares, posicionn);
				cargarEnVector((key_vector[posicionn].key),
						key_vector[lugares].data + prox_espacio, prox_espacio,
						true, posicionn);

			} else {
				return resultado = NULL;
				//	log_error(logger, "No es posible crear mas particiones");
					uint32_t aux = eliminar_particion(-1);
					actualizar_flags(lugares, aux);
					cargarEnVector((key_vector[aux].key),
							key_vector[lugares].data + prox_espacio, prox_espacio,
							true, aux);
//			log_debug(logger,"No hay posiciones libres en el vector");
//		    log_debug(logger, "particion_posicionn, flags: %d : %d \n",posicionn , key_vector[posicionn].flags);
			}
			pthread_rwlock_wrlock(keyVector);
			key_vector[lugares].data_size = prox_espacio;
			pthread_rwlock_unlock(keyVector);
			prox_particion = prox_espacio;
		} else {
			/*ya no lo puedo partir mas o mi espacio es mayor a la particion que cree.*/
//			pthread_rwlock_unlock(keyVector);
			pthread_rwlock_wrlock(keyVector);
			if (espacio > prox_particion)
				key_vector[lugares].data_unuse = key_vector[lugares].data_size
						- espacio;
			else
				key_vector[lugares].data_unuse = prox_particion - espacio;

			pthread_rwlock_unlock(keyVector);
			//	    log_debug(logger, "particion_lugares, flags: %d : %d \n",lugares , key_vector[lugares].flags);

			resultado = &key_vector[lugares];
			verdad = true;
		}
//		pthread_rwlock_rdlock(keyVector);

	} while (!verdad);

	return resultado;
}

key_element *buscarLibreBuddy(size_t espacio) {

	uint32_t i = 0;
	char encontrado = 0;
	uint32_t lugares = cantRegistros;

	pthread_rwlock_rdlock(keyVector);

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
					lugares = i;
					encontrado = 1;
				}
			} else
				//si llegue al final pero habia encontrado algo
				encontrado = 1;
		} else {
			if (((encontrado == 0) && (key_vector[i].data_size >= espacio))
					&& ((key_vector[i].libre))) {
				if (lugares == cantRegistros)
					lugares = i;
				else {
					if (key_vector[i].data_size
							<= key_vector[lugares].data_size)
						lugares = i;
				}
			}
		}
		i++;
	}

	pthread_rwlock_unlock(keyVector);

	key_element *resultado = crear_arbol(lugares, espacio);

	return resultado;
}

//busca el item con esa key y lo devuelve.
int32_t vector_get(char *key) {
//	key_element *resultado;
	int32_t resultado = -1;
	uint32_t encontrado = 0;
	uint32_t i = 0;
//buscar la key en el vector
	size_t nkey = strlen(key);
	pthread_rwlock_rdlock(keyVector);

	while ((encontrado == 0) && (i < cantRegistros)) {

		if ((i == cantRegistros) || key_vector[i].data_size == 0) {
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

