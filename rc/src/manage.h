/*
 * manage.h
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */
#include "our_engine.h"

#ifndef MANAGE_H_
#define MANAGE_H_

key_element *alocate_vector(void);
char *alocate_keys_space(void);
bool esPotenciaDe(uint32_t valor);
void logger_operation(const char *operation, const char *key);


#endif /* MANAGE_H_ */
