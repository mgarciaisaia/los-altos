/*
 * manage.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include "manage.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <src/commons/config.h>
#include <time.h>
#include <stdio.h>
#include <src/commons/log.h>
#include <string.h>

extern size_t cache_size;
extern size_t part_minima;
extern t_config* config;
uint32_t cantRegistros;

t_log *logger;

void logger_operation(const char *operation, const char *key) {
    log_debug(logger, "Operacion recibida: %s la key %s \n", operation, key);
}

bool esPotenciaDe(uint32_t valor) {

	bool resultado;
	double ptr;

	double resto = log(valor) / log(2);

	if (modf(resto, &ptr) == 0)
		resultado = true;
	else
		resultado = false;

	return resultado;

}

key_element *alocate_vector(void) {

	/* Calculo la cantidad total de registros */

	uint32_t resto = cache_size % part_minima;
	uint32_t cociente = cache_size / part_minima;
//	double ptr;
//	double aux = modf(worstCase, &ptr);

//	 uint32_t maxSize = 1;

	if (resto != 0)
		cantRegistros = (uint32_t) cociente + 1;
	else
		cantRegistros = (uint32_t) cociente;

	double cuenta = sizeof(key_element) * cantRegistros;
	key_element *key_table = malloc(cuenta);
	memset(key_table, 0, cuenta);
//	key_element *key_vector = key_table;

	return key_table;

}

char *alocate_keys_space(void) {

	char *resultado;
	int32_t MAX_KEY = config_get_int_value(config, "MAX_KEY");
	resultado = malloc(MAX_KEY * cantRegistros);
	return resultado;

}
