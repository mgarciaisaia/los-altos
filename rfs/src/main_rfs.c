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

//	struct Superblock *sb= read_superblock();
//	printf("primer bloque libre: %hu\n",sb->inode_size);

	//posicionarme en el comienzo de cada grupo, osea donde empieza el
	// bitmap block de c/u

//		uint8_t grupo = 0;
//
//		uint32_t *direccion = dir_inicioDeGrupo (grupo);
//
//		printf("direccion del grupo 1= %p, \n",direccion);


//		leerTablaDeInodos(0);

		//		struct GroupDesc *TDgrupo = leerGroupDescriptor(grupo);
//		printf("bloque de bitmap de grupo1: %d\n",TDgrupo->block_bitmap);

//		struct GroupDesc *TDgrupo2 = leerGroupDescriptor(3);
//		printf("bloque de bitmap de grupo3: %d\n",TDgrupo2->block_bitmap);

//	printf("inodos= %d, \n",bloque->inodes);
//	printf("inodos por grupo= %d, \n",bloque->inodes_per_group);
//	printf("bloques= %d, \n",bloque->blocks);
//	printf("bloques libres= %d, \n",bloque->free_blocks);
//	printf("inodos libres= %d, \n",bloque->free_inodes);
//	printf("bloques por grupo= %d, \n",bloque->blocks_per_group);
//	printf("primer bloque libre= %d, \n",bloque->first_data_block);
//	printf("bloques reservados= %d, \n",bloque->reserved_blocks);


/* Leer los descriptores de grupo */

//	leerLosGroupDescriptor(bloque->inodes,bloque->inodes_per_group);

//	struct GroupDesc * gd = leerGroupDescriptor(1);
//	printf("user dirs count: %hu\n",gd->used_dirs_count);
//	printf("block_bitmap: %hu\n",gd->block_bitmap);
//	printf("inode_bitmap: %hu\n",gd->inode_bitmap);
//	printf("inode_table: %hu\n",gd->inode_table);
//	leerGroupDescriptor(3);


//	leerBitmapDeBloque(0);

//	leerBitmapDeInodos(0);

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

/// Probando getInodo y bloques ocupados por un archivo
	uint32_t nroInodo = 12;
	struct INode * inodo = getInodo(nroInodo);
	bloquesOcupados(inodo);

//Fin - Probando getInodo/

	/* Aca liberamos la memoria que mapeamos */

//	leerIndirecciones(7);

//	printf("%d\n",nroBloqueInicioDeGrupo(0));

	extern uint32_t *ptr_arch;
	if (munmap(ptr_arch, sizeof(*ptr_arch)) == -1) {
	   	perror("Error un-mmapping the file");
	}

	return EXIT_SUCCESS;

}
