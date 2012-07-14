#ifndef ADMINISTRACION_H_
#define ADMINISTRACION_H_
#include "../commons/src/commons/collections/list.h"
#include "../commons/src/commons/collections/dictionary.h"
#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>

struct archivo_abierto {
    uint32_t numero_inodo;
    int cantidad_abiertos;
    pthread_rwlock_t *lock;
};

struct archivos_cliente {
    t_list *archivos;
    pthread_mutex_t *lock;
};

struct archivo_abierto *obtener_o_crear_archivo_abierto(t_list *archivos_abiertos, uint32_t numero_inodo);

void registrar_apertura(struct archivo_abierto *nodo_archivo);

void registrar_cierre(t_list *archivos_abiertos, struct archivo_abierto *nodo_archivo);

struct archivos_cliente *obtener_o_crear_lista_del_cliente(t_dictionary *archivos_por_cliente, u_int32_t client_id);

void registrar_apertura_cliente(struct archivos_cliente *archivos_cliente, u_int32_t numero_inodo);

void registrar_cierre_cliente(t_dictionary *archivos_por_cliente, uint32_t client_id, uint32_t numero_inodo);

void inicializar_administracion();

#endif /* ADMINISTRACION_H_ */
