/*
 * read_superBlock.c
 *
 *  Created on: 01/05/2012
 *      Author: utnso
 */

#include "funciones_rfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>
#include "src/sockets/sockets.h"
#include <netinet/in.h>
#include "src/nipc/nipc.h"
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include "src/nipc/nipc.h"
#include <fcntl.h>
#include <errno.h>
#include "src/commons/collections/list.h"
#include "src/commons/log.h"
#include "src/commons/config.h"
#include <semaphore.h>
#include "administracion.h"
#include <string.h>
#include <sys/inotify.h>
#include <arpa/inet.h>
#include <libmemcached/memcached.h>
#include "memcached_uses.h"
#include "../commons/src/commons/misc.h"
#include "../commons/src/commons/string.h"
#include <libgen.h>

#define MEMCACHED_KEY_SIZE 41
#define PATH_CONFIG "rfs.conf"
#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )

// El tamaño del buffer es igual a la cantidad maxima de eventos simultaneos
// que quiero manejar por el tamaño de cada uno de los eventos. En este caso
// Puedo manejar hasta 1024 eventos simultaneos.
#define BUF_LEN     ( 1024 * EVENT_SIZE )

int32_t sleep_time;
char *sleep_type;
t_log *logger;
int epoll;
uint16_t listening_port;
int max_connections;
int max_events;
sem_t threads_count;
t_list *archivos_abiertos;
uint32_t generated_client_id = 0;
pthread_mutex_t *mutex_client_id;
t_dictionary *archivos_por_cliente;
memcached_st *remote_cache;
bool cache_active;
int file_descriptor;

void avisar_socket_puerto(int socket, enum tipo_nipc tipo) {
	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;
	len = sizeof addr;
	int res = getsockname(socket, (struct sockaddr*) &addr, &len);
	if (res) {
		perror("getsockname");
		return;
	}
	// deal with both IPv4 and IPv6:
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *) &addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *) &addr;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}
	printf("******************************\n");
	printf("Socket: %d\n", socket);
	printf("IP address: %s\n", ipstr);
	printf("port: %d\n", port);
	printf("Tipo: %s\n", nombre_del_enum_nipc(tipo));
	printf("******************************\n");
}

void send_no_ok(int socket, int32_t errorCode, uint32_t client_id) {
	struct nipc_error *no_ok = new_nipc_error_code(client_id, -errorCode);
	nipc_send(socket, no_ok->serialize(no_ok));
}

void send_ok(int socket, uint32_t client_id) {
	struct nipc_packet *ok = new_nipc_ok(client_id);
	nipc_send(socket, ok);
}

void do_sleep(void) {

	//aca si pregunta por inotify a ver se cambia

	uint32_t micro = 1000000;
	uint32_t mili = 1000000000;

	if (strcmp(sleep_type, "MICRO") == 0)
		//en microsegundos
		usleep(sleep_time * micro);

	else
		//en milisegundos
		usleep(sleep_time * mili);

}


// FIXME: extraer a funciones_rfs.c
// FIXME: extraer a funciones_rfs.c
// FIXME: extraer a funciones_rfs.c
// FIXME: extraer a funciones_rfs.c



void *traerBloque(uint32_t numero_bloque) {
    void *bloque = NULL;
    if(cache_active) {
        bloque = traerBloqueDeCache(remote_cache, numero_bloque);
    }
    if(bloque == NULL) {
        bloque = traerBloqueDeDisco(numero_bloque);

        if(bloque == NULL) {
            // FIXME: romper aca - no pude leer ese bloque
        }
    }

    return bloque;
}

void grabarBloque(uint32_t numero_bloque, void *bloque) {
    if(cache_active) {
        guardarBloqueEnCache(remote_cache, numero_bloque, bloque);
    }
    void *bloque_disco = obtenerBloque(numero_bloque);
    int i;
    for(i = 0; i < tamanioDeBloque(); i++) {
        memcpy(bloque_disco + i, bloque + i, 1);
    }
}

/**
 *
 *
 * FIN FIXME extraer a funciones_rfs.c
 *
 *
 *
 */






/**
 * Registra la apertura del archivo en la lista de archivos abiertos
 * y en la lista de archivos del cliente
 */
void serve_open(int socket, struct nipc_open *request) {
	log_info(logger, "open %s con flags %o (pide %d)", request->path,
			request->flags, request->client_id);
	uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(request->path);
	if (numero_inodo != 0) {
		struct archivo_abierto *nodo_archivo = obtener_o_crear_archivo_abierto(
				archivos_abiertos, numero_inodo);
		registrar_apertura(nodo_archivo);
		struct archivos_cliente *archivos_cliente =
				obtener_o_crear_lista_del_cliente(archivos_por_cliente,
						request->client_id);
		registrar_apertura_cliente(archivos_cliente, numero_inodo);
		send_ok(socket, request->client_id);
	} else {
		send_no_ok(socket, ENOENT, request->client_id);
	}
	log_info(logger, "FIN open %s con flags %o (pide %d)", request->path,
			request->flags, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Si se creo el archivo, manda un nipc_ok, sino un nipc_error
 */
void serve_create(int socket, struct nipc_create *request) {
	log_debug(logger, "create %s mode %o (pide %d)", request->path,
			request->fileMode, request->client_id);

	do_sleep();

	int32_t codError = crearArchivo(request->path, request->fileMode);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else {
		uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(request->path);
		if (numero_inodo != 0) {
			struct archivo_abierto *nodo_archivo =
					obtener_o_crear_archivo_abierto(archivos_abiertos,
							numero_inodo);
			registrar_apertura(nodo_archivo);
			struct archivos_cliente *archivos_cliente =
					obtener_o_crear_lista_del_cliente(archivos_por_cliente,
							request->client_id);
			registrar_apertura_cliente(archivos_cliente, numero_inodo);
			send_ok(socket, request->client_id);
		} else {
			send_no_ok(socket, ENOENT, request->client_id);
		}
		send_ok(socket, request->client_id);
	}

	log_debug(logger, "FIN create %s mode %o (pide %d)", request->path,
			request->fileMode, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Contesta un nipc_read_response con los datos leidos, o nipc_error
 */
void serve_read(int socket, struct nipc_read *request) {
	log_debug(logger, "read %s @%d+%d (pide %d)", request->path, request->offset, request->size, request->client_id);

	void *output = NULL;
    size_t readBytes = 0;

	uint32_t numero_inodo = numeroInodo(request->path);
	bloquearLectura(numero_inodo);

	struct INode *inodoArchivo = obtenerInodo(numero_inodo);


	size_t bytesALeer = request->size;
	if(inodoArchivo->size < request->offset + request->size) {
	    bytesALeer = inodoArchivo->size - request->offset;
	    log_debug(logger, "Achico el pedido de %d bytes a %d para %s@%d", request->size, bytesALeer, request->path, request->offset);
	}

	if(bytesALeer) {

        t_list *numerosBloquesArchivo = numerosDeBloques(inodoArchivo, request->offset, bytesALeer);

        void **bloquesArchivo = calloc(numerosBloquesArchivo->elements_count, sizeof(void *));

        t_link_element *elemento = numerosBloquesArchivo->head;
        int indice = 0;
        while(elemento != NULL) {
            void *bloque = NULL;
            uint32_t numero_bloque = *(uint32_t *)elemento->data;
            if(cache_active) {
                bloque = traerBloqueDeCache(remote_cache, numero_bloque);
            }

            if(bloque == NULL) {
                bloque = traerBloqueDeDisco(numero_bloque);

                if(bloque == NULL) {
                    // FIXME: romper aca - no pude leer ese bloque
                    break;
                }

            if (cache_active) {
                guardarBloqueEnCache(remote_cache, numero_bloque, bloque);
            }
        }

            bloquesArchivo[indice++] = bloque;
            elemento = elemento->next;
        }

        output = malloc(numerosBloquesArchivo->elements_count * tamanioDeBloque());
        for(indice = 0; indice < numerosBloquesArchivo->elements_count; indice++) {
            size_t offsetBloque = indice == 0 ? desplazamientoDentroDelBloque(request->offset) : 0;
            size_t sizeALeer = indice == (numerosBloquesArchivo->elements_count - 1) ? desplazamientoDentroDelBloque(bytesALeer - 1) + 1 : tamanioDeBloque();
            memcpy(output + readBytes, bloquesArchivo[indice] + offsetBloque, sizeALeer);
            readBytes += sizeALeer;
            free(bloquesArchivo[indice]);
        }
        free(bloquesArchivo);
        list_destroy_and_destroy_elements(numerosBloquesArchivo, &free);
	}

	struct nipc_packet *response = new_nipc_read_response(output, readBytes, request->client_id);
    nipc_send(socket, response);

	desbloquear(numero_inodo);

	log_debug(logger, "FIN read %s @%d+%d (pide %d)", request->path, request->offset, bytesALeer, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_ok si pudo grabar, o un nipc_error
 */
void serve_write(int socket, struct nipc_write *request) {
    log_debug(logger, "write %s @%d+%d (pide %d)", request->path, request->offset, request->size, request->client_id);

    // FIXME: agregar do_sleep()
    uint32_t bytesEscritos = 0;
    uint32_t numero_inodo = numeroInodo(request->path);

    struct INode *inodoArchivo = obtenerInodo(numero_inodo);

    // FIXME: chequear errores (en todo el metodo?)

    bloquearEscritura(numero_inodo);

    uint32_t nroBloqueLogicoFinal = nroBloqueDentroDelInodo(request->offset + request->size - 1);

    while(cantidadDeBloques(inodoArchivo) <= nroBloqueLogicoFinal) {
        agregarBloqueNuevo(inodoArchivo);
    }

    t_list *numerosBloquesArchivo = numerosDeBloques(inodoArchivo, request->offset, request->size);

    t_link_element *elemento = numerosBloquesArchivo->head;
    while(elemento != NULL) {
        void *bloque = NULL;
        uint32_t numero_bloque = *(uint32_t *)elemento->data;
        uint32_t posicion_en_bloque = desplazamientoDentroDelBloque(request->offset + bytesEscritos);
        uint32_t bytes_a_escribir = tamanioDeBloque() - posicion_en_bloque;
        if(bytesEscritos + bytes_a_escribir > request->size) {
            bytes_a_escribir = request->size - bytesEscritos;
        }

        if(posicion_en_bloque || (tamanioDeBloque() - bytes_a_escribir)) {
            bloque = traerBloque(numero_bloque);
        } else {
            bloque = calloc(1, tamanioDeBloque());
        }

        int index;
        for(index = 0; index < bytes_a_escribir; index++) {
            memcpy(bloque + posicion_en_bloque + index, request->data + bytesEscritos + index, 1);
        }

        grabarBloque(numero_bloque, bloque);

        bytesEscritos += bytes_a_escribir;
        free(bloque);
        elemento = elemento->next;
    }

    list_destroy_and_destroy_elements(numerosBloquesArchivo, &free);

    // TODO: enviar bytesLeidos, no OK/ERROR
    if(bytesEscritos == request->size) {
        send_ok(socket, request->client_id);
    } else {
        send_no_ok(socket, 1, request->client_id);
    }

    desbloquear(numero_inodo);

    log_debug(logger, "FIN write %s @%d+%d (pide %d)", request->path, request->offset, request->size, request->client_id);
    free(request->path);
    free(request->data);
    free(request);
}

/**
 * Manda un nipc_ok si pudo hace release, o nipc_error
 * NOTA: FUSE dice ignorar el valor de retorno, si se complica contestar
 * contestemos cualquiera
 */
void serve_release(int socket, struct nipc_release *request) {
	log_info(logger, "release %s flags %o (pide %d)", request->path,
			request->flags, request->client_id);
	uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(request->path);
	if (numero_inodo != 0) {
		//FIXME:  si el archivo no esta abierto, romper
		struct archivo_abierto *nodo_archivo = obtener_o_crear_archivo_abierto(
				archivos_abiertos, numero_inodo);
		registrar_cierre(archivos_abiertos, nodo_archivo);
		registrar_cierre_cliente(archivos_por_cliente, request->client_id,
				numero_inodo);

		//		send_no_ok(socket, ERROR2);
		send_ok(socket, request->client_id);
	} else {
		send_no_ok(socket, ENOENT, request->client_id);
	}
	log_info(logger, "FIN release %s flags %o (pide %d)", request->path,
			request->flags, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_ok si pudo hacer unlink, o nipc_error
 */
void serve_unlink(int socket, struct nipc_unlink *request) {
	log_debug(logger, "unlink %s (pide %d)", request->path, request->client_id);

	uint32_t offset = 0;
	if (cache_active)
		delete_memcached(remote_cache, request->path, offset);

	do_sleep();

	int32_t codError = eliminarArchivo(request->path);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN unlink %s (pide %d)", request->path,
			request->client_id);
	free(request->path);
	free(request);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_mkdir(int socket, struct nipc_mkdir *request) {
	log_debug(logger, "mkdir %s modo %o (pide %d)", request->path,
			request->fileMode, request->client_id);

	do_sleep();

	int32_t codError = crearDirectorio(request->path, request->fileMode);

	if (codError != 0) {
		send_no_ok(socket, codError, request->client_id);
	} else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN mkdir %s modo %o (pide %d)", request->path,
			request->fileMode, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_readdir_response, o nipc_error
 */
void serve_readdir(int socket, struct nipc_readdir *request) {
	log_debug(logger, "readdir %s @%d (pide %d)", request->path,
			request->offset, request->client_id);

	do_sleep();

	t_list *entradas = listarDirectorio(request->path);

	if (!list_is_empty(entradas)) {
		/**
		 * La fruleada de aca tiene que tener una lista de entries del directorio
		 * Cada entry tiene el nombre/ruta relativa y los atributos (modo y size, como mínimo)
		 */
		int index = 0;
		struct readdir_entry *entries = calloc(entradas->elements_count,
				sizeof(struct readdir_entry));

		t_link_element *entry = entradas->head;
		for (index = 0; index < entradas->elements_count; index++) {
			memcpy(&(entries[index]), entry->data,
					sizeof(struct readdir_entry));
			entry = entry->next;
		}

		struct nipc_readdir_response *response = new_nipc_readdir_response(
				entradas->elements_count, entries, request->client_id);
		nipc_send(socket, response->serialize(response));
		free(response->entries);
		free(response);
		list_destroy_and_destroy_elements(entradas, &readdir_entry_destroy);

	} else
		send_no_ok(socket, ENOENT, request->client_id);

	log_debug(logger, "FIN readdir %s @%d (pide %d)", request->path,
			request->offset, request->client_id);
	free(request->path);
	free(request);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_rmdir(int socket, struct nipc_rmdir *request) {
	log_debug(logger, "rmdir %s (pide %d)", request->path, request->client_id);

//	uint32_t offset = 0;
//	if (cache_active)
//		delete_memcached(remote_cache, request->path, offset);

	do_sleep();

	int32_t codError = eliminarDirectorio(request->path);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN rmdir %s (pide %d)", request->path,
			request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_attr, o nipc_error
 */
void serve_getattr(int socket, struct nipc_getattr *request) {
	log_debug(logger, "getattr %s (pide %d)", request->path,
			request->client_id);
	char *path = request->path;

	do_sleep();

	struct INode *inodo = getInodoDeLaDireccionDelPath(path);
	if (inodo == NULL) {
		send_no_ok(socket, ENOENT, request->client_id);
		free(request->path);
		free(request);
		return;
	}
	struct readdir_entry *attributes = malloc(sizeof(struct readdir_entry));

	attributes->mode = inodo->mode;
	attributes->n_link = inodo->links;
	attributes->size = inodo->size;
	attributes->blocks = inodo->nr_blocks;

	struct nipc_getattr_response *response = new_nipc_getattr_response(
			attributes, request->client_id);
	nipc_send(socket, response->serialize(response));
	log_debug(logger, "FIN getattr %s (pide %d)", request->path,
			request->client_id);
	free(request->path);
	free(request);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_truncate(int socket, struct nipc_truncate *request) {
	log_debug(logger, "truncate %s a %d (pìde %d)", request->path, request->offset, request->client_id);

	do_sleep();

	int32_t codError = truncarArchivo(request->path, request->offset);
	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN truncate %s a %d (pìde %d)", request->path, request->offset, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_error porque no reconocimos el tipo
 */
void serve_unknown(int socket, struct nipc_packet *request) {
	log_debug(logger, "unknown de tipo %d, %d bytes (pide %d)", request->type,
			request->data_length, request->client_id);
	log_info(logger, "Llego un paquete de tipo desconocido: %d", request->type);
	printf("************* llego unknown en %d **************", socket);
	avisar_socket_puerto(socket, request->type);
	struct nipc_error *error = new_nipc_error_message("Paquete desconocido");
	nipc_send(socket, error->serialize(error));
	log_debug(logger, "FIN unknown de tipo %d, %d bytes (pide %d)",
			request->type, request->data_length, request->client_id);
	free(request->data);
	free(request);

}
uint32_t generate_client_id() {
	pthread_mutex_lock(mutex_client_id);
	uint32_t new_client_id = ++generated_client_id;
	pthread_mutex_unlock(mutex_client_id);
	return new_client_id;
}

void serve_handshake(int socket, struct nipc_packet *request) {
	log_debug(logger, "handshake");
	log_trace(logger, "handshake recibido: %s", request->data);
	if (!strcmp(request->data, HANDSHAKE_HELLO)) {
		int new_client_id = generate_client_id();
		nipc_send(socket, new_nipc_handshake_ok(new_client_id));
		log_info(logger, "Acepto un nuevo cliente con ID %d", new_client_id);
	} else {
		log_error(logger, "handshake: HELLO no reconocido (%d)", request->type);
		close(socket);
	}
	free(request->data);
	free(request);
	log_debug(logger, "/handshake");
}

void serve_error(int socket, struct nipc_error *request) {
	// FIXME manejar error (desconectado? error con codigo? mensaje?)
	log_error(logger, "Error %d en %d (cliente %d): %s", request->errorCode,
			socket, request->client_id, request->errorMessage);
	if (request->errorCode != -EBADF) {
		nipc_send(socket, request->serialize(request));
	} else {
		free(request->errorMessage);
		free(request);
	}
}

void *serveRequest(void *socketPointer) {
	int socket = *(int *) socketPointer;
	struct nipc_packet *request = nipc_receive(socket);
	//avisar_socket_puerto(socket, request->type);
	log_debug(logger, "Request en el socket %d con operacion %s\n", socket,
			nombre_del_enum_nipc(request->type));
	switch (request->type) {
	case nipc_open:
		serve_open(socket, deserialize_open(request));
		break;
	case nipc_create:
		serve_create(socket, deserialize_create(request));
		break;
	case nipc_read:
		serve_read(socket, deserialize_read(request));
		break;
	case nipc_write:
		serve_write(socket, deserialize_write(request));
		break;
	case nipc_release:
		serve_release(socket, deserialize_release(request));
		break;
	case nipc_unlink:
		serve_unlink(socket, deserialize_unlink(request));
		break;
	case nipc_mkdir:
		serve_mkdir(socket, deserialize_mkdir(request));
		break;
	case nipc_readdir:
		serve_readdir(socket, deserialize_readdir(request));
		break;
	case nipc_rmdir:
		serve_rmdir(socket, deserialize_rmdir(request));
		break;
	case nipc_getattr:
		serve_getattr(socket, deserialize_getattr(request));
		break;
	case nipc_truncate:
		serve_truncate(socket, deserialize_truncate(request));
		break;
	case nipc_handshake:
		serve_handshake(socket, request);
		break;
	case nipc_error:
		serve_error(socket, deserialize_error(request));
		break;
	default:
		serve_unknown(socket, request);
		break;
	}
	close(socket);
	sem_post(&threads_count);
	return NULL;
}

void *listen_config(void *nada) {

	log_debug(logger, "Escuchando cambio en config");

	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		perror("inotify_init");
	}
	// Creamos un monitor sobre un path indicando que eventos queremos escuchar
	int watch_descriptor = inotify_add_watch(file_descriptor, PATH_CONFIG,
			IN_MODIFY);

	char buffer[BUF_LEN];
	//aca si pregunta por inotify a ver se cambia

	while (1) {
		int length = read(file_descriptor, buffer, BUF_LEN);

		if (length < 0) {
			perror("read");
		}
		log_debug(logger, "length %d.\n", length);
		int offset = 0;

		while (offset < length) {
			struct inotify_event *event =
					(struct inotify_event *) &buffer[offset];

			if (event->len) {
				log_debug(logger, "event->len es CERO");
			}
			if (event->mask & IN_MODIFY) {
				if (event->mask & IN_ISDIR) {
					log_debug(logger, "The directory %s was modified.\n",
							event->name);
				} else {
//					log_debug(logger, "The file %s was modified.\n",
//							event->name);

					t_config *config = config_create(PATH_CONFIG);

					sleep_time = config_get_int_value(config, "sleep_time");
					sleep_type = strdup(config_get_string_value(config, "sleep_type"));

					config_destroy(config);

					offset += sizeof(struct inotify_event) + event->len;

				}
			}

			log_debug(logger, "sleep_time: %d , sleep_type: %s", sleep_time,
					sleep_type);

//			offset += sizeof(struct inotify_event) + event->len;
		}
	}
	inotify_rm_watch(file_descriptor, watch_descriptor);
	close(file_descriptor);

	return NULL;
}

void initialize_configuration() {
	t_config *config = config_create(PATH_CONFIG);

	char *log_level = config_get_string_value(config, "logger.level");
	char *log_file = config_get_string_value(config, "logger.file");
	char *log_name = config_get_string_value(config, "logger.name");
	int log_has_console = config_get_int_value(config, "logger.has_console");

	sleep_time = config_get_int_value(config, "sleep_time");
	sleep_type = strdup(config_get_string_value(config, "sleep_type"));

	logger = log_create(log_file, log_name, log_has_console,
			log_level_from_string(log_level));
	set_logger_funciones(logger);

	log_info(logger, "Inicializado el logger en %s con nivel %s", log_file,
			log_level);

	char *log_socket_level = config_get_string_value(config,
			"logger.sockets.level");
	char *log_socket_file = config_get_string_value(config,
			"logger.sockets.file");
	char *log_socket_name = config_get_string_value(config,
			"logger.sockets.name");
	int log_socket_has_console = config_get_int_value(config,
			"logger.sockets.has_console");

	socket_set_logger(
			log_create(log_socket_file, log_socket_name, log_socket_has_console,
					log_level_from_string(log_socket_level)));

	log_info(logger, "Inicializado el logger de sockets en %s con nivel %s",
			log_socket_file, log_socket_level);

	char *disk_path = config_get_string_value(config, "filesystem.disk");
	mapear_archivo(disk_path);
	log_info(logger, "Montado el disco ubicado en %s", disk_path);

	listening_port = config_get_int_value(config, "connections.listen_port");

	max_connections = config_get_int_value(config,
			"connections.max_connections");

	max_events = config_get_int_value(config, "connections.max_events");

	// 0 = semaforo compartido entre threads
	sem_init(&threads_count, 0,
			config_get_int_value(config, "connections.max_threads"));

	mutex_client_id = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex_client_id, NULL);

	// FIXME: con free() alcanza?
	archivos_por_cliente = dictionary_create(&free);

	inicializar_administracion();

	init_semaforos();

	cache_active = config_get_int_value(config, "is.active.cache");

	if (cache_active) {
		remote_cache = memcached_create(NULL);
		memcached_server_st *servers = NULL;
		memcached_return_t memcached_response;

		char *remote_cache_host = config_get_string_value(config, "cache.host");
		uint16_t remote_cache_port = config_get_int_value(config, "cache.port");
		servers = memcached_server_list_append(servers, remote_cache_host,
				remote_cache_port, &memcached_response);
		if (memcached_response != MEMCACHED_SUCCESS) {
			log_error(logger,
					"Error intentando agregar el servidor %s:%d a la cache: %s",
					remote_cache_host, remote_cache_port,
					memcached_strerror(remote_cache, memcached_response));
		} else {
			memcached_response = memcached_server_push(remote_cache, servers);
			if (memcached_response != MEMCACHED_SUCCESS) {
				log_error(logger,
						"Error intentando asignar los servidores a la cache: %s",
						memcached_strerror(remote_cache, memcached_response));
			} else {
				log_info(logger, "Conexion exitosa a la cache en %s:%d",
						remote_cache_host, remote_cache_port);
			}
		}
		set_memcached_utils_logger(logger);
		free(servers);
		// FIXME: hago free de servers o lo sigo necesitando?
	}

//		listen_config();
	pthread_t threadID;
	pthread_attr_t *thread_attributes = malloc(sizeof(pthread_attr_t));
    pthread_attr_init(thread_attributes);
    pthread_attr_setdetachstate(thread_attributes, PTHREAD_CREATE_DETACHED);
    pthread_create(&threadID, thread_attributes, &listen_config, NULL);
    pthread_detach(threadID);
    pthread_attr_destroy(thread_attributes);
    free(thread_attributes);
	//
	config_destroy(config);
}

int32_t main(void) {
	initialize_configuration();

	init_semaforos();

	archivos_abiertos = list_create();

	int listeningSocket = socket_binded(listening_port);
	struct sockaddr_in address;
	socklen_t addressLength = sizeof address;

	if (listen(listeningSocket, max_connections)) {
		perror("listen");
		close(listeningSocket);
		return -1;
	}
	log_debug(logger, "Escuchando el puerto %d", listening_port);

	epoll = epoll_create1(EPOLL_CLOEXEC);

	struct epoll_event *event = calloc(1, sizeof(struct epoll_event));
	event->data.fd = listeningSocket;
	event->events = EPOLLIN;

	epoll_ctl(epoll, EPOLL_CTL_ADD, listeningSocket, event);

	struct epoll_event *events = calloc(max_events, sizeof(struct epoll_event));

	while (1) { // roll, baby roll (8)

		int readySocketsCount = epoll_wait(epoll, events, max_events, -1);
		log_trace(logger, "Actividad del epoll");
		int index;
		for (index = 0; index < readySocketsCount; ++index) {
		    if ((events[index].events & EPOLLERR) || (events[index].events & EPOLLHUP) ||
		            (!(events[index].events & EPOLLIN))) {
                 fprintf (stderr, "epoll error\n");
                 close (events[index].data.fd);
                 continue;
               }
			if (events[index].data.fd == listeningSocket) {
				// nueva conexion entrante: la acepto y meto el nuevo descriptor en el poll
				int querySocket = accept(listeningSocket,
						(struct sockaddr *) &address, &addressLength);
				make_socket_non_blocking(querySocket);
				event->events = EPOLLIN | EPOLLET;
				event->data.fd = querySocket;
				epoll_ctl(epoll, EPOLL_CTL_ADD, querySocket, event);
			} else {
				log_trace(logger, "Actividad en un socket cualquiera");
				int querySocket = events[index].data.fd;
				epoll_ctl(epoll, EPOLL_CTL_DEL, querySocket, NULL);
				pthread_t threadID;
				sem_wait(&threads_count);
				pthread_attr_t *thread_attributes = malloc(
						sizeof(pthread_attr_t));
				pthread_attr_init(thread_attributes);
				pthread_attr_setdetachstate(thread_attributes,
						PTHREAD_CREATE_DETACHED);
				int pthread_return = pthread_create(&threadID, NULL,
						&serveRequest, &querySocket);
				pthread_detach(threadID);
				pthread_attr_destroy(thread_attributes);
				free(thread_attributes);
				if (pthread_return != 0) {
					printf("Wanda nara %s", strerror(pthread_return)); // FIXME: pimp my log
				}
			}
		}
	}

	destroy_semaforos();

	/* Aca liberamos la memoria que mapeamos */
	extern uint32_t *ptr_arch;
	if (munmap(ptr_arch, sizeof(*ptr_arch)) == -1) {
		perror("Error un-mmapping the file");
	}

	return EXIT_SUCCESS;
}


