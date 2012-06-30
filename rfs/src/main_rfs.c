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
#include <math.h>
#include "src/sockets/sockets.h"
#include <netinet/in.h>
#include "src/nipc/nipc.h"
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include "src/nipc/nipc.h"
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include "src/commons/collections/list.h"
#include "src/commons/log.h"
#include "src/commons/config.h"

// FIXME: esta ruta tiene que ser en el mismo path, y el makefile tiene que copiarlo
#define PATH_CONFIG "conf/rfs.conf"

t_log *logger;
int epoll;
u_int16_t listening_port;
int max_connections;
int max_events;


void send_ok(int socket) {
    struct nipc_packet *ok = new_nipc_ok();
    nipc_send(socket, ok);
}

/**
 * Si el path puede abrirse, manda un nipc_ok, sino un nipc_error
 */
void serve_open(int socket, struct nipc_open *request) {
    log_debug(logger, "open %s", request->path);
    // FIXME: contestar si puede o no abrir el archivo
    send_ok(socket);
    log_debug(logger, "/open %s", request->path);
}

/**
 * Si se creo el archivo, manda un nipc_ok, sino un nipc_error
 */
void serve_create(int socket, struct nipc_create *request) {
    log_debug(logger, "create %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/create %s", request->path);
}

/**
 * Contesta un nipc_read_response con los datos leidos, o nipc_error
 */
void serve_read(int socket, struct nipc_read *request) {
    log_debug(logger, "read %s", request->path);
    void *buffer;
    size_t readBytes = leerArchivo(request->path, request->offset, request->size, &buffer);

    struct nipc_packet *response = new_nipc_read_response(buffer, readBytes);
    nipc_send(socket, response);
    log_debug(logger, "/read %s", request->path);
}

/**
 * Manda un nipc_ok si pudo grabar, o un nipc_error
 */
void serve_write(int socket, struct nipc_write *request) {
    log_debug(logger, "write %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/write %s", request->path);
}

/**
 * Manda un nipc_ok si pudo hace release, o nipc_error
 * NOTA: FUSE dice ignorar el valor de retorno, si se complica contestar
 * contestemos cualquiera
 */
void serve_release(int socket, struct nipc_release *request) {
    log_debug(logger, "release %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/release %s", request->path);
}

/**
 * Manda un nipc_ok si pudo hacer unlink, o nipc_error
 */
void serve_unlink(int socket, struct nipc_unlink *request) {
    log_debug(logger, "unlink %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/unlink %s", request->path);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_mkdir(int socket, struct nipc_mkdir *request) {
    log_debug(logger, "mkdir %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/mkdir %s", request->path);
}

/**
 * Manda un nipc_readdir_response, o nipc_error
 */
void serve_readdir(int socket, struct nipc_readdir *request) {
    log_debug(logger, "readdir %s", request->path);
    t_list *entradas = listarDirectorio(request->path);
    /**
     * La fruleada de aca tiene que tener una lista de entries del directorio
     * Cada entry tiene el nombre/ruta relativa y los atributos (modo y size, como mínimo)
     */
    int index = 0;
    struct readdir_entry *entries = calloc(entradas->elements_count, sizeof(struct readdir_entry));

    t_link_element *entry = entradas->head;
    for(index = 0; index < entradas->elements_count; index++) {
        memcpy(&(entries[index]), entry->data, sizeof(struct readdir_entry));
        entry = entry->next;
    }

    // FIXME: manejar la falla de permisos (con /lost+found estaba rompiendo el server)

    struct nipc_readdir_response *response = new_nipc_readdir_response(entradas->elements_count, entries);
    nipc_send(socket, response->serialize(response));
    log_debug(logger, "/readdir %s", request->path);
    list_destroy_and_destroy_elements(entradas, &readdir_entry_destroy);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_rmdir(int socket, struct nipc_rmdir *request) {
    log_debug(logger, "rmdir %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/rmdir %s", request->path);
}

/**
 * Manda un nipc_attr, o nipc_error
 */
void serve_getattr(int socket, struct nipc_getattr *request) {
    log_debug(logger, "getattr %s", request->path);
    char *path = request->path;
    struct INode *inodo = getInodoDeLaDireccionDelPath(path);
    if(inodo == NULL) {
        nipc_send(socket, new_getattr_error(-ENOENT));
        return;
    }
    struct readdir_entry *attributes = malloc(sizeof(struct readdir_entry));

    attributes->mode = inodo->mode;
    attributes->n_link = inodo->links;
    attributes->size = inodo->size;

    struct nipc_getattr_response *response = new_nipc_getattr_response(attributes);
    nipc_send(socket, response->serialize(response));
    log_debug(logger, "/getattr %s", request->path);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_truncate(int socket, struct nipc_truncate *request) {
    log_debug(logger, "truncate %s", request->path);
    // FIXME: implementar
    send_ok(socket);
    log_debug(logger, "/truncate %s", request->path);
}

/**
 * Manda un nipc_error porque no reconocimos el tipo
 */
void serve_unknown(int socket, struct nipc_packet *request) {
    log_debug(logger, "unknown de tipo %d", request->type);
    log_info(logger, "Llego un paquete de tipo desconocido: %d", request->type);
    struct nipc_packet *error = new_nipc_error("Paquete desconocido");
    nipc_send(socket, error);
    log_debug(logger, "/unknown de tipo %d", request->type);
}

void serve_handshake(int socket, struct nipc_packet *request) {
    log_debug(logger, "handshake");
    log_trace(logger, "handshake recibido: %s", request->data);
    if(!strcmp(request->data, HANDSHAKE_HELLO)) {
        nipc_send(socket, new_nipc_handshake_ok());
    } else {
        log_error(logger, "handshake: HELLO no reconocido (%d)", request->type);
        close(socket);
    }
    log_debug(logger, "/handshake");
}

void serve_disconnected(int socket, struct nipc_packet *request) {
    log_debug(logger, "desconectado del servidor");
    close(socket);
}

void *serveRequest(void *socketPointer) {
    int socket = *(int *) socketPointer;
    log_debug(logger, "Nuevo request en el socket %d", socket);
    struct nipc_packet *request = nipc_receive(socket);
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
    case nipc_disconnected:
        serve_disconnected(socket, request);
        break;
    default:
        serve_unknown(socket, request);
        break;
    }
    epoll_ctl(epoll, EPOLL_CTL_DEL, socket, NULL);
    return NULL;
}

void initialize_configuration() {
    t_config *config = config_create(PATH_CONFIG);

    char *log_level = config_get_string_value(config, "logger.level");
    char *log_file = config_get_string_value(config, "logger.file");
    char *log_name = config_get_string_value(config, "logger.name");
    int log_has_console = config_get_int_value(config, "logger.has_console");

    logger = log_create(log_file, log_name, log_has_console, log_level_from_string(log_level));

    log_info(logger, "Inicializado el logger en %s con nivel %s", log_file, log_level);

    char *log_socket_level = config_get_string_value(config, "logger.sockets.level");
    char *log_socket_file = config_get_string_value(config, "logger.sockets.file");
    char *log_socket_name = config_get_string_value(config, "logger.sockets.name");
    int log_socket_has_console = config_get_int_value(config, "logger.sockets.has_console");

    socket_set_logger(log_create(log_socket_file, log_socket_name, log_socket_has_console, log_level_from_string(log_socket_level)));

    log_info(logger, "Inicializado el logger de sockets en %s con nivel %s", log_socket_file, log_socket_level);


    char *disk_path = config_get_string_value(config, "filesystem.disk");
    mapear_archivo(disk_path);
    log_info(logger, "Montado el disco ubicado en %s", disk_path);

    listening_port = config_get_int_value(config, "connections.listen_port");

    max_connections = config_get_int_value(config, "connections.max_connections");

    max_events = config_get_int_value(config, "connections.max_events");

    config_destroy(config);
}

int32_t main(void) {
    initialize_configuration();

    int accepted = 0;
    int listeningSocket = socket_binded(listening_port);
    struct sockaddr_in address;
    socklen_t addressLength = sizeof address;

    if(listen(listeningSocket, max_connections)) {
        perror("listen");
        close(listeningSocket);
        return -1;
    }
    log_debug(logger, "Escuchando el puerto %d", listening_port);

    epoll = epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event event;
    event.data.fd = listeningSocket;
    event.events = EPOLLIN;

    epoll_ctl(epoll, EPOLL_CTL_ADD, listeningSocket, &event);

    struct epoll_event *events = calloc(max_connections, sizeof event);

    while (1) { // roll, baby roll (8)
        int readySocketsCount = epoll_wait(epoll, events, max_events, -1);
        log_debug(logger, "Actividad del epoll");
        int index;
        for (index = 0; index < readySocketsCount; ++index) {
            if (events[index].data.fd == listeningSocket) {
                // nueva conexion entrante: la acepto y meto el nuevo descriptor en el poll
                int querySocket = accept(listeningSocket,
                        (struct sockaddr *) &address, &addressLength);
                log_debug(logger, "Acepto una nueva conexion (van %d)", accepted++);
                // FIXME:
                // setnonblocking(querySocket);
                int flags = fcntl(querySocket, F_GETFL, 0);
                if(flags < 0) {
                    flags = 0;
                }
                if(fcntl(querySocket, F_SETFD, flags | O_NONBLOCK)) {
                   perror("fcntl");
                   return -1;
                }
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = querySocket;
                epoll_ctl(epoll, EPOLL_CTL_ADD, querySocket, &event);
            } else {
                log_debug(logger, "Actividad en un socket cualquiera");
                int querySocket = events[index].data.fd;
                pthread_t threadID;
                pthread_attr_t threadAttributes;
                pthread_attr_init(&threadAttributes);
                pthread_create(&threadID, &threadAttributes, &serveRequest, &querySocket);
            }
        }
    }

    return 0;

//	read_superblock();

// Mostrar los descriptores de grupo

//	uint32_t cantGrupos = cantidadDeGrupos();
//	int nroGrupo;
//	for(nroGrupo = 0;nroGrupo < cantGrupos;nroGrupo++){
//		printf("grupo nro: %d\n: ",nroGrupo);
//		leerGroupDescriptor(nroGrupo);
//	}

// Fin - Mostrar los descriptores de grupo

// Busqueda de bloques libres

//	t_queue * bloquesLibres = buscarBloquesLibres(4);
//	if (!queue_is_empty(bloquesLibres)){
//		printf("entro aca\n");
//		printf("tam: %d\n",queue_size(bloquesLibres));
//		queue_clean(bloquesLibres);
//		printf("tam: %d\n",queue_size(bloquesLibres));
//		queue_destroy(bloquesLibres);
//	}

//Fin - Busqueda de bloques libres

// Busqueda inodos libres

//	t_queue * inodosLibres = buscarInodosLibres(4);
//	if (!queue_is_empty(inodosLibres)){
//		printf("entro aca\n");
//		printf("tam: %d\n",queue_size(inodosLibres));
//		queue_clean(inodosLibres);
//		printf("tam: %d\n",queue_size(inodosLibres));
//		queue_destroy(inodosLibres);
//	}

//Fin - Busqueda inodos libres

// Probando getInodo y lectura de bloques ocupados por un archivo

//	uint32_t nroInodo = 12;
//	struct INode * inodo = getInodo(nroInodo);
//	leerBloquesInodos(inodo);

//Fin - Probando getInodo y lectura de bloques ocupados por un archivo

// Listar directorio

//	uint32_t nroInodoRaiz = 2;
//	struct INode * inodo = getInodo(nroInodoRaiz);
//	leerDirectorio(inodo);

	leerDirectorio("carpeta1/carpeta2");

// Fin - Listar directorio

	/* Aca liberamos la memoria que mapeamos */
	extern uint32_t *ptr_arch;
	if (munmap(ptr_arch, sizeof(*ptr_arch)) == -1) {
	   	perror("Error un-mmapping the file");
	}

	return EXIT_SUCCESS;

}
