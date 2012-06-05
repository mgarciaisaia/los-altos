/*
 * manage.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */
#include "our_engine.h"
#include "manage.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#define MAX_KEY 41

uint32_t cantRegistros;

void alocate_buddy(void) {

}

key_element *alocate_keysDinam(double worstCase) {

	double cuenta = (sizeof(key_element) + MAX_KEY) * worstCase;

	void *key_table = malloc(cuenta);

	key_element *key_vector = key_table;

	/* Calculo la cantidad total de registros */

	double ptr;
	double aux = modf(worstCase, &ptr);

//	 uint32_t maxSize = 1;

	if (aux != 0)
		cantRegistros = (uint32_t) ptr + 1;
	else
		cantRegistros = (uint32_t) ptr;

	return key_vector;

}
