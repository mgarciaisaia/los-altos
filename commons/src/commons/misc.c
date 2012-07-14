#include "misc.h"
#include <stdint.h>
#include <stdlib.h>

uint32_t *duplicar_uint32(uint32_t numero) {
    uint32_t *copia = malloc(sizeof(uint32_t));
    *copia = numero;
    return copia;
}
