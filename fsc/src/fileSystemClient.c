// Antes de incluir nada, para que no lo pisen
#define FUSE_USE_VERSION 27

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <sys/types.h>
#include "src/nipc/nipc.h"
#include "src/sockets/sockets.h"
#include <errno.h>
#include <string.h>

//FIXME: parametrizar
#define fileSystemIP "127.0.0.1"
#define fileSystemPort 3087


/**
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 */
int check_error(const char *operation, const char *path, struct nipc_packet *response) {
    if(response->type == nipc_error) {
        printf("%s %s: %s\n", operation, path, (char*)response->data);
        return -1;
    } else {
        printf("%s %s: Paquete invalido (%d)\n", operation, path, response->type);
        return -1;
    }
}

int check_ok_error(const char *operation, const char *path, struct nipc_packet *response) {
    if(response->type != nipc_ok) {
        return check_error(operation, path, response);
    }
    return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int remote_create(const char *path, mode_t mode, struct fuse_file_info *fileInfo) {
    struct nipc_create* createData = new_nipc_create(path, mode);
    struct nipc_packet* packet = createData->serialize(createData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("create", path, response);
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
/**
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Open()
 *
 * int hello_open(const char *path, struct fuse_file_info *fi)
 *
 * Argument     in/out              description
 * path         input               A path to file we want to check for opening possibility.
 * fi           input and output    a structure containing detailed information about operation
 * return       output              zero if everything went OK, negeted error otherwise
 *
 */
int remote_open(const char *path, struct fuse_file_info *fileInfo) {
    struct nipc_open* openData = new_nipc_open(path, fileInfo->flags);
    struct nipc_packet* packet = openData->serialize(openData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("open", path, response);
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
/**
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Read()
 *
 * read(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
 *
 * Argument     in/out      Description
 * path         input       A path to the file from which function has to read
 * buf          output      buffer to which function has to write contents of the file
 * size         input       both the size of buf and amount of data to read
 * offset       input       offset from the beginning of file
 * fi           input       detailed information about read operation, see fuse_file_info for more information
 * return       output      amount of bytes read, or negated error number on error
 */
int remote_read(const char *path, char *output, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    struct nipc_read* readData = new_nipc_read(path, size, offset);
    struct nipc_packet* packet = readData->serialize(readData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    if(response->type == nipc_read_response) {
        size_t readBytes = (response->data_length < size) ? response->data_length : size;
        memcpy(output, response->data, readBytes);
        return readBytes;
    } else {
        return check_error("read", path, response);
    }
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int remote_write(const char *path, const char *input, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    struct nipc_write* writeData = new_nipc_write(path, input, size, offset);
    struct nipc_packet* packet = writeData->serialize(writeData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    int failed = check_ok_error("write", path, response);
    if(failed) {
        return failed;
    } else {
        return size;
    }
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int remote_release(const char *path, struct fuse_file_info *fileInfo) {
    struct nipc_release* releaseData = new_nipc_release(path, fileInfo->flags);
    struct nipc_packet* packet = releaseData->serialize(releaseData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("release", path, response);
}

/** Remove a file */
int remote_unlink(const char *path) {
    printf("%d, %s\n", nipc_unlink, path);
    struct nipc_unlink* unlinkData = new_nipc_unlink(path);
    struct nipc_packet* packet = unlinkData->serialize(unlinkData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("unlink", path, response);
}

/** Create a directory */
int remote_mkdir(const char *path, mode_t mode) {
    printf("%d, %d, %s\n", nipc_mkdir, mode, path);
    struct nipc_mkdir* mkdirData = new_nipc_mkdir(path, mode);
    struct nipc_packet* packet = mkdirData->serialize(mkdirData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("mkdir", path, response);
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
/**
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Readdir()
 *
 * int readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
 *
 * Argument   in/out    Description
 * path       input     A path to the directory we want information about
 * buf        output    buffer containing information about directory
 * filler     input     pointer to fuse_fill_dir_t function that is used to add entries to buffer
 * offset     input     offset in directory entries, read below for more information
 * fi         input     A struct fuse_file_info, contains detailed information why this readdir operation was invoked.
 * return     output    negated error number, or 0 if everything went OK
 */
int remote_readdir(const char *path, void *output, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
    printf("%d, %s, %lu\n", nipc_readdir, path, offset);
    struct nipc_readdir* readdirData = new_nipc_readdir(path, offset);
    struct nipc_packet* packet = readdirData->serialize(readdirData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    if(response->type == nipc_readdir_response) {
        struct nipc_readdir_response *readdirData = deserialize_readdir_response(response);
        int index;
        int bufferIsFull = 0;

        for(index = 0; index < readdirData->entriesLength && !bufferIsFull; index++) {
            struct readdir_entry *entry = &(readdirData->entries[index]);
            struct stat stats;
            stats.st_nlink = entry->n_link;
            stats.st_mode = entry->mode;
            stats.st_size = entry->size;
            bufferIsFull = filler(output, entry->path, &stats, 0);
        }

        if(bufferIsFull) {
            perror("Buffer de readdir lleno");
            // FIXME: aca devuelvo -1 o como se maneja el buffer lleno?
            return -1;
        }

        return 0;
    } else {
        return check_error("readdir", path, response);
    }
}

/** Remove a directory */
int remote_rmdir(const char *path) {
    printf("%d, %s\n", nipc_rmdir, path);
    struct nipc_rmdir* rmdirData = new_nipc_rmdir(path);
    struct nipc_packet* packet = rmdirData->serialize(rmdirData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("rmdir", path, response);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
/**
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Getattr()
 *
 * int getattr (const char *path, struct stat *stbuf)
 *
 * Argument in/out  Description
 * path    input    A path to the file that we want information about.
 * stbuf   output   A struct stat that the information should be stored in.
 * return  output   negated error number or 0 if everything went OK
 *
 */
int remote_getattr(const char *path, struct stat *statbuf) {
    printf("%d, %s\n", nipc_getattr, path);
    struct nipc_getattr* getattrData = new_nipc_getattr(path);
    struct nipc_packet* packet = getattrData->serialize(getattrData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    if(response->type == nipc_getattr_response) {
        struct nipc_getattr_response *getattr = deserialize_getattr_response(response);

        statbuf->st_mode = getattr->entry->mode;
        statbuf->st_nlink = getattr->entry->n_link;
        statbuf->st_size = getattr->entry->size;

        return 0;
    } else if(response->type == nipc_getattr_error) {
        return *(int*)response->data;
    } else {
        return check_error("getattr", path, response);
    }
}

/**
 * Change the size of a file
 */
int remote_truncate(const char * path, off_t offset) {
    printf("%d, %s, %lu\n", nipc_truncate, path, offset);
    struct nipc_truncate* truncateData = new_nipc_truncate(path, offset);
    struct nipc_packet* packet = truncateData->serialize(truncateData);
    struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
    return check_ok_error("truncate", path, response);
}

/**
 * http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Functions_list
 */
static struct fuse_operations remote_operations = {
		.create = remote_create,
		.open = remote_open,
		.read = remote_read,
		.write = remote_write,
		.release = remote_release,
		.unlink = remote_unlink,
		.mkdir = remote_mkdir,
		.readdir = remote_readdir,
		.rmdir = remote_rmdir,
		.getattr = remote_getattr,
		.truncate = remote_truncate
};

int remote_handshake() {
    struct nipc_packet* response = nipc_query(new_nipc_handshake_hello(), fileSystemIP, fileSystemPort);
    if(response->type == nipc_handshake) {
        if(strcmp(response->data, HANDSHAKE_OK)) {
            printf("handshake: respuesta no reconocida\n");
            return -1;
        } else {
            printf("handshake: ok");
            return 0;
        }
    } else {
        return check_error("handshake", "", response);
    }
}

int main(int argc, char *argv[]) {
    // FIXME: parametros de configuracion
    if(remote_handshake()) {
        return -1;
    }
	return fuse_main(argc, argv, &remote_operations, NULL);
}
