#include <stdlib.h>
#include "administracion.h"
#include "../commons/src/commons/collections/list.h"
#include "../commons/src/commons/collections/dictionary.h"
#include "../commons/src/commons/string.h"
#include "../commons/src/commons/misc.h"

pthread_mutex_t *lock_archivos_abiertos;
pthread_mutex_t *lock_archivos_clientes;

void inicializar_administracion() {
    lock_archivos_abiertos = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lock_archivos_abiertos, NULL);
    lock_archivos_clientes = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lock_archivos_clientes, NULL);
}

struct archivo_abierto *obtener_o_crear_archivo_abierto(t_list *archivos_abiertos, uint32_t numero_inodo) {
    struct archivo_abierto *nodo = NULL;
    pthread_mutex_lock(lock_archivos_abiertos);
    t_link_element *element = archivos_abiertos->head;
    // Yeap, esto es un list_find. Pero no se cuán felices son las inner functions...
    while(element != NULL && ((struct archivo_abierto *)element->data)->numero_inodo != numero_inodo) {
        element = element->next;
    }

    if(element == NULL) {
        nodo = malloc(sizeof(struct archivo_abierto));
        nodo->numero_inodo = numero_inodo;
        nodo->cantidad_abiertos = 0;
        nodo->lock = malloc(sizeof(pthread_rwlock_t));
        pthread_rwlock_init(nodo->lock, NULL);
        list_add(archivos_abiertos, nodo);
    } else {
        nodo = (struct archivo_abierto *)element->data;
    }
    pthread_mutex_unlock(lock_archivos_abiertos);

    return nodo;
}

void registrar_apertura(struct archivo_abierto *nodo_archivo) {
    // XXX: podriamos chequear el overflow de cantidad_abiertos si nos fuera a corregir gente muy despiadada
    pthread_rwlock_wrlock(nodo_archivo->lock);
    nodo_archivo->cantidad_abiertos++;
    pthread_rwlock_unlock(nodo_archivo->lock);
}

void registrar_cierre(t_list *archivos_abiertos, struct archivo_abierto *nodo_archivo) {
    // TODO: valor de retorno?
    pthread_rwlock_wrlock(nodo_archivo->lock);
    if(nodo_archivo->cantidad_abiertos > 0) {
        nodo_archivo->cantidad_abiertos--;
        if(nodo_archivo->cantidad_abiertos == 0) {
            int index;
            t_link_element *element = archivos_abiertos->head;
            for(index = 0; index < archivos_abiertos->elements_count; index++) {
                if(element != NULL && ((struct archivo_abierto *)element->data) == nodo_archivo) {
                    struct archivo_abierto *nodo = list_remove(archivos_abiertos, index);
                    free(nodo->lock);
                    free(nodo);
                    return;
                }
                element = element->next;
            }
        }
    } else {
        // FIXME: aca tiramos un error de intentar cerrar un archivo que no estaba abierto
    }
    pthread_rwlock_unlock(nodo_archivo->lock);
}

struct archivos_cliente *obtener_o_crear_lista_del_cliente(t_dictionary *archivos_por_cliente, uint32_t client_id) {

    pthread_mutex_lock(lock_archivos_clientes);
    char *str_client_id = string_from_uint32(client_id);
    struct archivos_cliente *archivos_cliente = (struct archivos_cliente *) dictionary_get(archivos_por_cliente, str_client_id);
    if(archivos_cliente == NULL) {
        archivos_cliente = malloc(sizeof(struct archivos_cliente));
        archivos_cliente->lock = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(archivos_cliente->lock, NULL);
        archivos_cliente->archivos = list_create();
        dictionary_put(archivos_por_cliente, str_client_id, archivos_cliente);
    } else {
        free(str_client_id); // Si hice el put, no tengo que free'ear la clave
    }
    pthread_mutex_unlock(lock_archivos_clientes);
    return archivos_cliente;
}

void registrar_apertura_cliente(struct archivos_cliente *archivos_cliente, uint32_t numero_inodo) {
    pthread_mutex_lock(archivos_cliente->lock);
    list_add(archivos_cliente->archivos, duplicar_uint32(numero_inodo));
    pthread_mutex_unlock(archivos_cliente->lock);
}

void registrar_cierre_cliente(t_dictionary *archivos_por_cliente, uint32_t client_id, uint32_t numero_inodo) {
    char *str_client_id = string_from_uint32(client_id);
    pthread_mutex_lock(lock_archivos_clientes);
    struct archivos_cliente *archivos_cliente = dictionary_get(archivos_por_cliente, str_client_id);
    pthread_mutex_unlock(lock_archivos_clientes);

    pthread_mutex_lock(archivos_cliente->lock);
    int index;
    t_link_element *element = archivos_cliente->archivos->head;
    for(index = 0; index < archivos_cliente->archivos->elements_count; index++) {
        if(element != NULL && *((uint32_t *)element->data) == numero_inodo) {
            free(list_remove(archivos_cliente->archivos, index));
            continue;
        }
        element = element->next;
    }
    pthread_mutex_unlock(archivos_cliente->lock);

    pthread_mutex_lock(lock_archivos_clientes);
    if(archivos_cliente->archivos->elements_count < 1) {
        struct archivos_cliente* archivos_cliente = dictionary_remove(archivos_por_cliente, str_client_id);
        list_destroy(archivos_cliente->archivos);
        pthread_mutex_destroy(archivos_cliente->lock);
        free(archivos_cliente->lock);
        free(archivos_cliente);
    }
    pthread_mutex_unlock(lock_archivos_clientes);
    free(str_client_id);
}
