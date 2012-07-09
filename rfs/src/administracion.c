#include <stdlib.h>
#include "administracion.h"
#include "../commons/src/commons/collections/list.h"
#include <string.h>

struct archivo_abierto *obtener_o_crear_archivo_abierto(t_list *archivos_abiertos, uint32_t numero_inodo) {
    // Yeap, esto es un list_find. Pero no se cuán felices son las inner functions...
    struct archivo_abierto *nodo = NULL;
    t_link_element *element = archivos_abiertos->head;
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
                    free(list_remove(archivos_abiertos, index));
                    continue;
                }
                element = element->next;
            }
        }
    } else {
        // FIXME: aca tiramos un error de intentar cerrar un archivo que no estaba abierto
    }
    pthread_rwlock_unlock(nodo_archivo->lock);
}
