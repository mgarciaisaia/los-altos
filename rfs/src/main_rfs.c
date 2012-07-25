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
#include <sys/types.h>
#include <errno.h>
#include "src/commons/collections/list.h"
#include "src/commons/log.h"
#include "src/commons/config.h"
#include <semaphore.h>
#include "administracion.h"
#include <string.h>
#include <sys/inotify.h>
#include <arpa/inet.h>

#define PATH_CONFIG "rfs.conf"

int32_t sleep_time;
char *sleep_type;
t_log *logger;
int epoll;
u_int16_t listening_port;
int max_connections;
int max_events;
sem_t threads_count;
t_list *archivos_abiertos;
u_int32_t generated_client_id = 0;
pthread_mutex_t *mutex_client_id;
t_dictionary *archivos_por_cliente;

void avisar_socket_puerto(int socket, enum tipo_nipc tipo) {
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;
    len = sizeof addr;
    getsockname(socket, (struct sockaddr*)&addr, &len);
    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
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
	struct nipc_packet *no_ok = new_getattr_error(-errorCode, client_id);
	nipc_send(socket, no_ok);
}

void send_ok(int socket, uint32_t client_id) {
	struct nipc_packet *ok = new_nipc_ok(client_id);
	nipc_send(socket, ok);
}

void do_sleep(void){

	//aca si pregunta por inotify a ver se cambia
//	int file_descriptor = inotify_init();
//		if (file_descriptor < 0) {
//			perror("inotify_init");
//		}
//	// Creamos un monitor sobre un path indicando que eventos queremos escuchar
//	int watch_descriptor = inotify_add_watch(file_descriptor, PATH_CONFIG, IN_MODIFY);
//
//
//
	//aca si pregunta por inotify a ver se cambia
	if (strcmp(sleep_type, "MICRO") == 0)
	//en microsegundos
		usleep(sleep_time);
	else
	//en milisegundos
		usleep(sleep_time*1000);

}


/**
 * Registra la apertura del archivo en la lista de archivos abiertos
 * y en la lista de archivos del cliente
 */
void serve_open(int socket, struct nipc_open *request) {
	log_info(logger, "open %s con flags %o (pide %d)", request->path, request->flags, request->client_id);
	uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(request->path);
	if (numero_inodo != 0) {
		struct archivo_abierto *nodo_archivo = obtener_o_crear_archivo_abierto(
				archivos_abiertos, numero_inodo);
		registrar_apertura(nodo_archivo);
        struct archivos_cliente *archivos_cliente = obtener_o_crear_lista_del_cliente(archivos_por_cliente, request->client_id);
        registrar_apertura_cliente(archivos_cliente, numero_inodo);
		send_ok(socket, request->client_id);
	} else {
	    send_no_ok(socket, ENOENT, request->client_id);
	}
	log_info(logger, "FIN open %s con flags %o (pide %d)", request->path, request->flags, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Si se creo el archivo, manda un nipc_ok, sino un nipc_error
 */
void serve_create(int socket, struct nipc_create *request) {
	log_debug(logger, "create %s mode %o (pide %d)", request->path, request->fileMode, request->client_id);

	do_sleep();

	int32_t codError = crearArchivo(request->path, request->fileMode);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN create %s mode %o (pide %d)", request->path, request->fileMode, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Contesta un nipc_read_response con los datos leidos, o nipc_error
 */
void serve_read(int socket, struct nipc_read *request) {
	log_debug(logger, "read %s @%d+%d (pide %d)", request->path, request->offset, request->size, request->client_id);
	void *buffer;

	do_sleep();

	size_t readBytes = leerArchivo(request->path, request->offset,
			request->size, &buffer);

	if (buffer == NULL && readBytes != 0) {
		send_no_ok(socket, readBytes, request->client_id);
	} else {
		struct nipc_packet *response = new_nipc_read_response(buffer,
				readBytes, request->client_id);
		nipc_send(socket, response);
	}
	log_debug(logger, "FIN read %s @%d+%d (pide %d)", request->path, request->offset, request->size, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_ok si pudo grabar, o un nipc_error
 */
void serve_write(int socket, struct nipc_write *request) {
	log_debug(logger, "write %s @%d+%d (pide %d): %s", request->path, request->offset, request->size, request->client_id, request->data);

	do_sleep();

	int32_t codError = escribirArchivo(request->path, request->data,
			request->size, request->offset);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

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
	log_info(logger, "release %s flags %o (pide %d)", request->path, request->flags, request->client_id);
	uint32_t numero_inodo = getNroInodoDeLaDireccionDelPath(request->path);
	if (numero_inodo != 0) {
		//FIXME:  si el archivo no esta abierto, romper
		struct archivo_abierto *nodo_archivo = obtener_o_crear_archivo_abierto(
				archivos_abiertos, numero_inodo);
		registrar_cierre(archivos_abiertos, nodo_archivo);
        registrar_cierre_cliente(archivos_por_cliente, request->client_id, numero_inodo);

		//		send_no_ok(socket, ERROR2);
		send_ok(socket, request->client_id);
	} else {
		send_no_ok(socket, ENOENT, request->client_id);
	}
	log_info(logger, "FIN release %s flags %o (pide %d)", request->path, request->flags, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_ok si pudo hacer unlink, o nipc_error
 */
void serve_unlink(int socket, struct nipc_unlink *request) {
	log_debug(logger, "unlink %s (pide %d)", request->path, request->client_id);

	do_sleep();

	int32_t codError = eliminarArchivo(request->path);

	if (codError != 0)
		send_no_ok(socket, codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN unlink %s (pide %d)", request->path, request->client_id);
	free(request->path);
	free(request);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_mkdir(int socket, struct nipc_mkdir *request) {
	log_debug(logger, "mkdir %s modo %o (pide %d)", request->path, request->fileMode, request->client_id);

	do_sleep();

	int32_t codError = crearDirectorio(request->path,request->fileMode);

	if (codError != 0) {
		send_no_ok(socket, codError, request->client_id);
	} else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN mkdir %s modo %o (pide %d)", request->path, request->fileMode, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_readdir_response, o nipc_error
 */
void serve_readdir(int socket, struct nipc_readdir *request) {
	log_debug(logger, "readdir %s @%d (pide %d)", request->path, request->offset, request->client_id);

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
				entradas->elements_count, entries, request->client_id );
		nipc_send(socket, response->serialize(response));
		free(response->entries);
		free(response);
		list_destroy_and_destroy_elements(entradas, &readdir_entry_destroy);

	} else
		send_no_ok(socket, ENOENT, request->client_id);

	log_debug(logger, "FIN readdir %s @%d (pide %d)", request->path, request->offset, request->client_id);
	free(request->path);
	free(request);
}

/**
 * nipc_ok, o nipc_error
 */
void serve_rmdir(int socket, struct nipc_rmdir *request) {
	log_debug(logger, "rmdir %s (pide %d)", request->path, request->client_id);

	do_sleep();

	int32_t codError = eliminarDirectorio(request->path);

	if (codError != 0)
		send_no_ok(socket,codError, request->client_id);
	else
		send_ok(socket, request->client_id);

	log_debug(logger, "FIN rmdir %s (pide %d)", request->path, request->client_id);
	free(request->path);
	free(request);
}

/**
 * Manda un nipc_attr, o nipc_error
 */
void serve_getattr(int socket, struct nipc_getattr *request) {
	log_debug(logger, "getattr %s (pide %d)", request->path, request->client_id);
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

	struct nipc_getattr_response *response = new_nipc_getattr_response(
			attributes, request->client_id);
	nipc_send(socket, response->serialize(response));
	log_debug(logger, "FIN getattr %s (pide %d)", request->path, request->client_id);
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
		send_no_ok(socket,codError, request->client_id);
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
	log_debug(logger, "unknown de tipo %d, %d bytes (pide %d)", request->type, request->data_length, request->client_id);
	log_info(logger, "Llego un paquete de tipo desconocido: %d", request->type);
	printf("************* llego unknown en %d **************", socket);
	avisar_socket_puerto(socket, request->type);
	struct nipc_packet *error = new_nipc_error(strdup("Paquete desconocido"));
	nipc_send(socket, error);
	log_debug(logger, "FIN unknown de tipo %d, %d bytes (pide %d)", request->type, request->data_length, request->client_id);
	free(request->data);
	free(request);

}
u_int32_t generate_client_id() {
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

void serve_disconnected(int socket, struct nipc_packet *request) {
	log_debug(logger, "desconectado del servidor");
	printf("************* desconectado en %d **************", socket);
	free(request->data);
	free(request);
	close(socket);
}

void *serveRequest(void *socketPointer) {
	int socket = *(int *) socketPointer;
	struct nipc_packet *request = nipc_receive(socket);
	avisar_socket_puerto(socket, request->type);
	log_debug(logger, "Request en el socket %d con operacion %s\n", socket, nombre_del_enum_nipc(request->type));
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
	close(socket);
	sem_post(&threads_count);
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
    sem_init(&threads_count, 0, config_get_int_value(config, "connections.max_threads"));

    mutex_client_id = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_client_id, NULL);

    // FIXME: con free() alcanza?
    archivos_por_cliente = dictionary_create(&free);

    inicializar_administracion();

    init_semaforos();

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
		log_debug(logger, "Actividad del epoll");
		int index;
		for (index = 0; index < readySocketsCount; ++index) {
			if (events[index].data.fd == listeningSocket) {
				// nueva conexion entrante: la acepto y meto el nuevo descriptor en el poll
				int querySocket = accept(listeningSocket,
						(struct sockaddr *) &address, &addressLength);
				if(querySocket<0) {
				    perror("Accept fallo");
				    return -1;
				}
				// FIXME:
				// setnonblocking(querySocket);
				int flags = fcntl(querySocket, F_GETFL, 0);
				if (flags < 0) {
					flags = 0;
				}
				if (fcntl(querySocket, F_SETFD, flags | O_NONBLOCK)) {
					perror("fcntl");
					return -1;
				}
				event->events = EPOLLIN | EPOLLET;
				event->data.fd = querySocket;
				epoll_ctl(epoll, EPOLL_CTL_ADD, querySocket, event);
			} else {
				log_debug(logger, "Actividad en un socket cualquiera");
				int querySocket = events[index].data.fd;
				epoll_ctl(epoll, EPOLL_CTL_DEL, querySocket, NULL);
				pthread_t threadID;
				sem_wait(&threads_count);
				pthread_attr_t *thread_attributes = malloc(sizeof(pthread_attr_t));
				pthread_attr_init(thread_attributes);
				pthread_attr_setdetachstate(thread_attributes, PTHREAD_CREATE_DETACHED);
                int pthread_return = pthread_create(&threadID, NULL, &serveRequest, &querySocket);
                pthread_detach(threadID);
                pthread_attr_destroy(thread_attributes);
                free(thread_attributes);
                if(pthread_return != 0) {
                    printf("Wanda nara %s", strerror(pthread_return)); // FIXME: pimp my log
                }
			}
		}
	}

	destroy_semaforos();
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

//	leerDirectorio("carpeta1/carpeta2");

// Fin - Listar directorio

	/* Aca liberamos la memoria que mapeamos */
	extern uint32_t *ptr_arch;
	if (munmap(ptr_arch, sizeof(*ptr_arch)) == -1) {
		perror("Error un-mmapping the file");
	}

	return EXIT_SUCCESS;

}
