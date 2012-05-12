/*
 * funciones_rfs.h
 *
 *  Created on: 03/05/2012
 *      Author: utnso
 */


#ifndef FUNCIONES_RFS_H_
#define FUNCIONES_RFS_H_

#include <string.h>
#include <stdint.h>



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

	struct GroupDesc
	{
	    uint32_t block_bitmap;
	    uint32_t inode_bitmap;
	    uint32_t inode_table;
	    uint16_t free_blocks_count;
	    uint16_t free_inodes_count;
	    uint16_t used_dirs_count;
	    uint16_t dummy[7];
	};

	struct INode
	{
	    uint16_t mode;
	    uint16_t uid;
	    uint32_t size;
	    uint32_t atime;
	    uint32_t ctime;
	    uint32_t mtime;
	    uint32_t dtime;
	    uint16_t gid;
	    uint16_t links;
	    uint32_t nr_blocks;
	    uint32_t flags;
	    uint32_t reserved1;
	    uint32_t blocks[12];
	    uint32_t iblock;
	    uint32_t iiblock;
	    uint32_t iiiblock;
	    uint32_t generation;
	    uint32_t file_acl;
	    uint32_t size_hi;
	    uint32_t reserved2[4];
	};

	struct CampoBits {
	    unsigned int b0:1;
	    unsigned int b1:1;
	    unsigned int b2:1;
	    unsigned int b3:1;
	    unsigned int b4:1;
	    unsigned int b5:1;
	    unsigned int b6:1;
	    unsigned int b7:1;
	};

	void mapear_archivo();
	struct Superblock *read_superblock();

	uint32_t cantidadDeGrupos(uint32_t,uint32_t);
	void leerLosGroupDescriptor(uint32_t,uint32_t);

	struct GroupDesc * leerGroupDescriptor(uint32_t);


//	char esPotenciaDe(uint32_t grupo);
	uint8_t *dir_inicioDeGrupo (uint32_t grupo);
	struct GroupDesc *leerGroupDescriptor(uint32_t);

	void leerBitmapDeBloque(uint32_t);
	void leerBitmapDeInodos(uint32_t);

	void leerTablaDeInodos(uint32_t nroGrupo);

#endif /* FUNCIONES_RFS_H_ */
