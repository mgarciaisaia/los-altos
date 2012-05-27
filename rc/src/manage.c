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

 key_element *key_vector;

void alocate_keysDinam(double worstCase){

	double cuenta = 50 * worstCase;

	void *key_table = malloc(cuenta);

	key_vector = key_table;

	double ptr;
	double aux = modf(worstCase, &ptr);

	uint32_t maxSize = 1;

	if (aux != 0)
		maxSize = (uint32_t) ptr + 1;
	else
		maxSize = (uint32_t) ptr;
}
