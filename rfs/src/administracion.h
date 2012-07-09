#ifndef ADMINISTRACION_H_
#define ADMINISTRACION_H_
#include "../commons/src/commons/collections/list.h"
#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>

struct archivo_abierto {
    uint32_t numero_inodo;
    int cantidad_abiertos;
    pthread_rwlock_t *lock;
};

struct archivo_abierto *obtener_o_crear_archivo_abierto(t_list *archivos_abiertos, uint32_t numero_inodo);

void registrar_apertura(struct archivo_abierto *nodo_archivo);

void registrar_cierre(t_list *archivos_abiertos, struct archivo_abierto *nodo_archivo);

#endif /* ADMINISTRACION_H_ */
