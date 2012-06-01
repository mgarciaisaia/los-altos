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

void alocate_buddy(void){

}

key_element *alocate_keysDinam(double worstCase) {

	double cuenta = 50 * worstCase;

	void *key_table = malloc(cuenta);

	key_element *key_vector = key_table;

return key_vector;

	/* Esto no es necesario

	 double ptr;
	 double aux = modf(worstCase, &ptr);

	 uint32_t maxSize = 1;

	 if (aux != 0)
	 maxSize = (uint32_t) ptr + 1;
	 else
	 maxSize = (uint32_t) ptr;
	 */
}
