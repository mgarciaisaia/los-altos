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

int32_t main(void) {


	uint32_t *datos = mapear_archivo();

	struct Superblock *bloque = read_superblock(datos);




/* aca seria el main *****************************************/

	printf("inodos= %d, \n",bloque->inodes);
	printf("inodos por grupo= %d, \n",bloque->inodes_per_group);
	printf("bloques= %d, \n",bloque->blocks);
	printf("bloques libres= %d, \n",bloque->free_blocks);
	printf("inodos libres= %d, \n",bloque->free_inodes);
	printf("bloques por grupo= %d, \n",bloque->blocks_per_group);
	printf("primer bloque libre= %d, \n",bloque->first_data_block);
	printf("bloques reservados= %d, \n",bloque->reserved_blocks);

/* Leer los descriptores de grupo */

	leerLosGroupDescriptor(datos,bloque->inodes,bloque->inodes_per_group);

	/* Aca liberamos la memoria que mapeamos */

	if (munmap(datos, sizeof(datos)) == -1) {
	   	perror("Error un-mmapping the file");
	}

	return EXIT_SUCCESS;

}

