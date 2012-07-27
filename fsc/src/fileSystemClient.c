// Antes de incluir nada, para que no lo pisen
#define FUSE_USE_VERSION 27

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "src/nipc/nipc.h"
#include "src/sockets/sockets.h"
#include <errno.h>
#include <string.h>
#include "src/commons/log.h"
#include "src/commons/config.h"
#include <libmemcached/memcached.h>
#include "memcached_utils.h"

#define PATH_CONFIG "fsc.conf"
#define MEMCACHED_KEY_SIZE 41

t_log *logger;
char *fileSystemIP;
uint16_t fileSystemPort;
int maximumReadWriteSize = 32 * 1024;
memcached_st *remote_cache;
bool cache_active;
uint32_t client_id = 0;


/**
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 * FIXME: falta hacer free() a todos los response
 */
void logger_operation(const char *operation, const char *path) {
	log_debug(logger, "Operacion recibida: %s en %s", operation, path);
}

void logger_operation_read_write(const char *operation, const char *path,
		size_t size, off_t offset) {
	log_debug(logger, "Operacion recibida: %s en %s (@%lu, %lu bytes)",
			operation, path, offset, size);
}

int check_error(const char *operation, const char *path, struct nipc_packet *packet) {
    if(packet->type != nipc_error) {
        log_error(logger, "Paquete desconocido de tipo %d: %.*s", packet->type, packet->data_length, packet->data);
        if(packet->data_length > 0) {
            free(packet->data);
        }
        free(packet);
        return -1;
    }
    struct nipc_error *response = deserialize_error(packet);
    log_error(logger, "Error %d al hacer %s %s: %s (%s)", response->errorCode, operation, path, response->errorMessage, strerror(-response->errorCode));
    int32_t return_value = response->errorCode;
    if(return_value == 0) {
        return_value = -1;
    }

    free(response->errorMessage);
    free(response);
    return return_value;
}

int check_ok_error(const char *operation, const char *path, struct nipc_packet *response) {
	if (response->type == nipc_ok) {
        free(response);
        return 0;
	}
    return check_error(operation, path, response);
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
    logger_operation("create", path);
    struct nipc_create* createData = new_nipc_create(client_id, path, mode);
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
    logger_operation("open", path);
    struct nipc_open* openData = new_nipc_open(client_id, path, fileInfo->flags);
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
int remote_read(const char *path, char *output, size_t size, off_t offset,
		struct fuse_file_info *fileInfo) {
	logger_operation_read_write("read", path, size, offset);

	// FIXME revisar como se maneja la cache y todo eso
//    memcached_return_t *memcached_response = memcached_add(remote_cache, "hola", strlen("hola"), "hola puto :)", strlen("hola puto :)"), 0, 0);
//    if(memcached_response != MEMCACHED_SUCCESS) {
//        printf("NOOOOO %s\n", memcached_strerror(remote_cache, memcached_response));
//    }

	int bytesRead = 0;

	while(bytesRead < size) {
	    int requestSize = size - bytesRead;
	    if(requestSize > maximumReadWriteSize) {
	        requestSize = maximumReadWriteSize;
	    }

        struct nipc_read* readData = new_nipc_read(client_id, path, requestSize, offset + bytesRead);
        struct nipc_packet* packet = readData->serialize(readData);
        struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
        if (response->type == nipc_read_response) {
            if(response->data_length == 0) {
                free(response);
                break;
            }
            memcpy(output + bytesRead, response->data, response->data_length);
            bytesRead += response->data_length;
            free(response->data);
            free(response);
        } else {
            return check_error("read", path, response);
        }
	}

	logger_operation_read_write("FIN read", path, bytesRead, offset);

    return bytesRead;
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
int remote_write(const char *path, const char *input, size_t size, off_t offset,
		struct fuse_file_info *fileInfo) {
	logger_operation_read_write("write", path, size, offset);

	int bytesWrote = 0;

    while(bytesWrote < size) {
        int requestSize = size - bytesWrote;
        if(requestSize > maximumReadWriteSize) {
            requestSize = maximumReadWriteSize;
        }

        struct nipc_write *writeData = new_nipc_write(client_id, path, input, requestSize, offset + bytesWrote);
        struct nipc_packet* packet = writeData->serialize(writeData);
        struct nipc_packet* response = nipc_query(packet, fileSystemIP, fileSystemPort);
        if (response->type == nipc_ok) {
            free(response);
            bytesWrote += requestSize;
        } else {
            return check_error("write", path, response);
        }
    }

    logger_operation_read_write("FIN write", path, bytesWrote, offset);

    return bytesWrote;
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
	logger_operation("release", path);
	struct nipc_release* releaseData = new_nipc_release(client_id, path, fileInfo->flags);
	struct nipc_packet* packet = releaseData->serialize(releaseData);
	struct nipc_packet* response = nipc_query(packet, fileSystemIP,
			fileSystemPort);
	return check_ok_error("release", path, response);
}

/** Remove a file */
int remote_unlink(const char *path) {
	logger_operation("unlink", path);
	struct nipc_unlink* unlinkData = new_nipc_unlink(client_id, path);
	struct nipc_packet* packet = unlinkData->serialize(unlinkData);
	struct nipc_packet* response = nipc_query(packet, fileSystemIP,
			fileSystemPort);
	return check_ok_error("unlink", path, response);
}

/** Create a directory */
int remote_mkdir(const char *path, mode_t mode) {
	logger_operation("mkdir", path);
	struct nipc_mkdir* mkdirData = new_nipc_mkdir(client_id, path, mode);
	struct nipc_packet* packet = mkdirData->serialize(mkdirData);
	struct nipc_packet* response = nipc_query(packet, fileSystemIP,
			fileSystemPort);
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

	logger_operation("readdir", path);

	struct nipc_packet *response = NULL;

	if (cache_active) {
	    response = query_memcached(remote_cache, path, nipc_readdir_response, client_id);
	}

	if(response == NULL) {
	    struct nipc_readdir* readdirData = new_nipc_readdir(client_id, path, offset);
        struct nipc_packet* packet = readdirData->serialize(readdirData);
        response = nipc_query(packet, fileSystemIP, fileSystemPort);

        if(response->type != nipc_readdir_response) {
            return check_error("readdir", path, response);
        }

        if(cache_active) {
            store_memcached(remote_cache, path, response);
        }
	}

	struct nipc_readdir_response *readdirData = deserialize_readdir_response(response);

	int index;
    int bufferIsFull = 0;

    for (index = 0; index < readdirData->entriesLength && !bufferIsFull; index++) {
        struct readdir_entry *entry = &(readdirData->entries[index]);
        struct stat stats;
        stats.st_nlink = entry->n_link;
        stats.st_mode = entry->mode;
        stats.st_size = entry->size;
        bufferIsFull = filler(output, entry->path, &stats, 0);
        free(entry->path);
    }

    free(readdirData->entries);
    free(readdirData);

    if (bufferIsFull) {
        log_info(logger, "readdir: buffer lleno");
        /*
         * FIXME
         *
         * En <http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/unclear.html> dicen
         * que cuando se llena el buffer se debe devolver ENOMEM
         *
         * En <http://sourceforge.net/apps/mediawiki/fuse/index.php?title=Readdir%28%29>
         * hablan de prestar atencion al offset, para cuando es muy grande la cantidad
         * de entradas.
         *
         * Habria que averiguar si hay que contemplar estos casos o no
         */
    }

    return 0;
}

/** Remove a directory */
int remote_rmdir(const char *path) {
	logger_operation("rmdir", path);
	struct nipc_rmdir* rmdirData = new_nipc_rmdir(client_id, path);
	struct nipc_packet* packet = rmdirData->serialize(rmdirData);
	struct nipc_packet* response = nipc_query(packet, fileSystemIP,
			fileSystemPort);
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
    logger_operation("getattr", path);

    struct nipc_packet *response = NULL;

    if (cache_active) {
        response = query_memcached(remote_cache, path, nipc_getattr_response, client_id);
    }

    if(response == NULL) {
        struct nipc_getattr* getattrData = new_nipc_getattr(client_id, path);
        struct nipc_packet* packet = getattrData->serialize(getattrData);
        response = nipc_query(packet, fileSystemIP, fileSystemPort);

        if(response->type != nipc_getattr_response) {
            return check_error("getattr", path, response);
        }

        if(cache_active) {
            store_memcached(remote_cache, path, response);
        }
    }

    struct nipc_getattr_response *getattr = deserialize_getattr_response(response);

    statbuf->st_mode = getattr->entry->mode;
    statbuf->st_nlink = getattr->entry->n_link;
    statbuf->st_size = getattr->entry->size;

    // No hago free() de getattr->entry->path porque en getattr ignoramos el path
    // (es siempre NULL - se usa en readdir)
    free(getattr->entry);
    free(getattr);

    return 0;
}

/**
 * Change the size of a file
 */
int remote_truncate(const char * path, off_t offset) {
	logger_operation("truncate", path);
	struct nipc_truncate* truncateData = new_nipc_truncate(client_id, path, offset);
	struct nipc_packet* packet = truncateData->serialize(truncateData);
	struct nipc_packet* response = nipc_query(packet, fileSystemIP,
			fileSystemPort);
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
	struct nipc_packet* response = nipc_query(new_nipc_handshake_hello(),
			fileSystemIP, fileSystemPort);
	if (response->type == nipc_handshake) {
		if (strcmp(response->data, HANDSHAKE_OK)) {
			log_error(logger, "handshake: respuesta no reconocida");
			void *validData = malloc(response->data_length);
			strncpy(validData, response->data, response->data_length);
			log_debug(logger, "hanshake: %d bytes %s", response->data_length,
					response->data);
			free(validData);
			free(response->data);
            free(response);
			return -1;
		} else {
            client_id = response->client_id;
            log_info(logger, "Handshake: conectado con Client ID %d", client_id);
            free(response->data);
            free(response);
			return 0;
		}
	} else {
	    free(response->data);
        free(response);
		return check_error("handshake", "", response);
	}
}

void initialize_configuration() {
	t_config *config = config_create(PATH_CONFIG);

	char *log_level = config_get_string_value(config, "logger.level");
	char *log_file = config_get_string_value(config, "logger.file");
	char *log_name = config_get_string_value(config, "logger.name");
	int log_has_console = config_get_int_value(config, "logger.has_console");

	logger = log_create(log_file, log_name, log_has_console,
			log_level_from_string(log_level));

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

	fileSystemIP = strdup(config_get_string_value(config, "filesystem.ip"));
	fileSystemPort = config_get_int_value(config, "filesystem.port");
	log_info(logger, "Ubicacion del FileSystem: %s:%u", fileSystemIP,
			fileSystemPort);

	maximumReadWriteSize = config_get_int_value(config, "readwrite.maxsize")
			* 1024;
	log_info(logger, "Tamano maximo de bloque para entrada/salida: %d",
			maximumReadWriteSize);

	cache_active = config_get_int_value(config, "is.active.cache");

	if(cache_active) {
	    remote_cache = memcached_create(NULL);
        memcached_server_st *servers = NULL;
        memcached_return_t memcached_response;

	    char *remote_cache_host = config_get_string_value(config, "cache.host");
        uint16_t remote_cache_port = config_get_int_value(config, "cache.port");
        servers = memcached_server_list_append(servers, remote_cache_host, remote_cache_port, &memcached_response);
        if (memcached_response != MEMCACHED_SUCCESS) {
            log_error(logger,
                    "Error intentando agregar el servidor %s:%d a la cache: %s",
                    remote_cache_host, remote_cache_port,
                    memcached_strerror(remote_cache, memcached_response));
        } else {
            memcached_response = memcached_server_push(remote_cache, servers);
            if (memcached_response != MEMCACHED_SUCCESS) {
                log_error(logger,
                        "Error intentando asignar los servidores a la cache: %s",
                        memcached_strerror(remote_cache, memcached_response));
            } else {
                log_info(logger, "Conexion exitosa a la cache en %s:%d",
                        remote_cache_host, remote_cache_port);
            }
        }
        set_memcached_utils_logger(logger);
        free(servers);
        // FIXME: hago free de servers o lo sigo necesitando?
	}

	config_destroy(config);
}

int main(int argc, char *argv[]) {
	initialize_configuration();

	if (logger == NULL) {
		perror("No hay logger");
		return -1;
	}
	if (remote_handshake()) {
		socket_destroy_logger();
		log_destroy(logger);
		return -1;
	}
	log_debug(logger, "Corro fuse_main");
	int fuseReturn = fuse_main(argc, argv, &remote_operations, NULL);
	log_debug(logger, "Termina el fsc: %d - %s", fuseReturn, strerror(errno));
	// FIXME: avisarle al rfs que se cierra
	socket_destroy_logger();
	log_destroy(logger);
	return fuseReturn;
}
