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


void serve_open(int socket, struct nipc_open *request) {
    printf("SEEEEEEEEEEEEEEEEE!!!!!!!!!");
    nipc_send(socket, request->serialize(request));
    printf("Path: %s\nType: %d\nFlags: %d\n", request->path, request->nipcType,
            request->flags);
    // FIXME: comprobar que pueda abrir el archivo
    // devuelve 0 si esta OK, sino devuelve -1
}

void serve_create(int newSocket, struct nipc_create *packet) {

}

void serveRequest(void *socketPointer) {
    int socket = *(int *) socketPointer;
    // crear un nuevo thread en el que atender
    struct nipc_packet *request = nipc_receive(socket);
    switch (request->type) {
    case nipc_open:
        serve_open(socket, deserialize_open(request));
        break;
    case nipc_create:
        serve_create(socket, deserialize_create(request));
        break;
        // FIXME: etc...
    default:
        // FIXME: Boom! no reconocimos el tipo de paquete
        printf("Me llego un tipo de paquete que no conozco: %d", request->type);
        break;
    }
}

int32_t main(void) {
    int listeningSocket = socket_binded(3087); // FIXME: parametrizar
#define MAX_CONNECTIONS 4
#define MAX_EVENTS 64 // FIXME: no tengo idea de por que este numero
    struct sockaddr_in address;
    socklen_t addressLength = sizeof address;

    listen(listeningSocket, MAX_CONNECTIONS);

    int epoll = epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event event;
    event.data.fd = listeningSocket;
    event.events = EPOLLIN;

    epoll_ctl(epoll, EPOLL_CTL_ADD, listeningSocket, &event);

    struct epoll_event *events = calloc(MAX_CONNECTIONS, sizeof event);

    while (1) { // roll, baby roll (8)
        int readySocketsCount = epoll_wait(epoll, events, MAX_EVENTS, -1);
        int index;
        for (index = 0; index < readySocketsCount; ++index) {
            if (events[index].data.fd == listeningSocket) {
                // nueva conexion entrante: la acepto y meto el nuevo descriptor en el poll
                int querySocket = accept(listeningSocket,
                        (struct sockaddr *) &address, &addressLength);
                // FIXME: setnonblocking(querySocket);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = querySocket;
                epoll_ctl(epoll, EPOLL_CTL_ADD, querySocket, &event);
            } else {
                int querySocket = events[index].data.fd;
                pthread_t threadID;
                pthread_attr_t threadAttributes;
                pthread_attr_init(&threadAttributes);
                pthread_create(&threadID, &threadAttributes, &serveRequest,
                        &querySocket);
            }
        }
    }

    return 0;
    mapear_archivo();

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
