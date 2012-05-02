/*
 * read_superBlock.c
 *
 *  Created on: 01/05/2012
 *      Author: utnso
 */

#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

#define PATH "/home/utnso/Desarrollo/ext2.disk"
#define OFFSET 256

int32_t main(void) {

	struct Superblock
	{
	    uint32_t inodes;
	    uint32_t blocks;
	    uint32_t reserved_blocks;
	    uint32_t free_blocks;
	    uint32_t free_inodes;
	    uint32_t first_data_block;
	    uint32_t log_block_size;
	    uint32_t fragment_size;
	    uint32_t blocks_per_group;
	    uint32_t fragments_per_group;
	    uint32_t inodes_per_group;
	    uint32_t mount_time;
	    uint32_t write_time;
	    uint16_t mount_count;
	    uint16_t max_mount;
	    uint16_t magic;
	    uint16_t state;
	    uint16_t errors;
	    uint16_t minor;
	    uint32_t last_check;
	    uint32_t check_int;
	    uint32_t creator_os;
	    uint32_t rev_level;
	    uint16_t resv_uid;
	    uint16_t resv_gid;
	};

	char archivo= open(PATH, O_RDONLY);
	if (archivo == -1) {
		perror("Error opening file for reading");
		exit(EXIT_FAILURE);
	}

	uint32_t offset_inicio = 0;

	uint32_t *p= mmap(NULL, 4096, PROT_READ, MAP_SHARED, archivo, offset_inicio);
	if (p == MAP_FAILED) {
		close(archivo);
		perror("Error mapping the file");
		exit(EXIT_FAILURE);
	}

	// defino el offset (addr) apartir del cual voy a leer
	// n bytes (len)

		uint32_t len = 1024;

		if (posix_madvise(p, len, POSIX_MADV_NORMAL) < 0)
	        perror("posix_madvise");

	/* leemos los datos al archivo */
	uint32_t *datos =p;

	struct Superblock *bloque;

		uint32_t *posicion = datos+OFFSET;
		bloque= (struct Superblock *) posicion;

	printf("inodos= %d, \n",bloque->inodes);
	printf("bloques= %d, \n",bloque->blocks);
	printf("bloques libres= %d, \n",bloque->free_blocks);
	printf("inodos libres= %d, \n",bloque->free_inodes);
	printf("bloques por grupo= %d, \n",bloque->blocks_per_group);
	printf("primer bloque libre= %d, \n",bloque->first_data_block);
	printf("bloques reservados= %d, \n",bloque->reserved_blocks);

	/* Aca liberamos la memoria que mapeamos */
	//munmap(p, TAMSECTOR);

	if (munmap(p, sizeof(datos)) == -1) {
	   	perror("Error un-mmapping the file");
	}
	close(archivo);

	return EXIT_SUCCESS;

}

