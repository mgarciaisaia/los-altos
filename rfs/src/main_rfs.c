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


void serve_open(int socket, struct nipc_open *request) {
    char *archivo = request->path;
    printf("SEEEEEEEEEEEEEEEEE!!!!!!!!!");
    socket_send(socket,request);
    printf("Path: %s\nType: %d\nFlags: %d\n", request->path, request->nipcType, request->flags);
    // FIXME: comprobar que pueda abrir el archivo
    // devuelve 0 si esta OK, sino devuelve -1
}

int32_t main(void) {

    int socketDescriptor = socket_binded(3087);
    struct sockaddr_in address;
    socklen_t addressLength = sizeof address;
    while(1) { // FIXME: aca iria el select o lo que pinte :)
        listen(socketDescriptor, 1);
        int newSocket = accept(socketDescriptor, (struct sockaddr *)&address, &addressLength);
        void* message = NULL;
        int messageSize = socket_receive(newSocket,&message);
        struct nipc_packet *packet = nipc_deserialize(message,messageSize);
        switch(packet->type) {
            case nipc_open:
                serve_open(newSocket, deserialize_open(packet));
                break;
            case nipc_create:
                serve_create(newSocket, deserialize_create(packet));
                break;
            // FIXME: etc...
            default:
                // FIXME: Boom! no reconocimos el tipo de paquete
                printf("Me llego un tipo de paquete que no conozco: %d", packet->type);
                break;
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
