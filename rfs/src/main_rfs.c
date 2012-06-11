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


int32_t main(void) {


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
