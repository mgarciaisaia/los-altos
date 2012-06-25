#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "nipc.h"

static struct nipc_packet* serialize_create(struct nipc_create* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1; //(el +1 es para el \0)

    // El largo de los datos es el largo del path + lo que ocupa el modo
    packet->data_length = path_lenght + sizeof(mode_t);

    packet->data = malloc(packet->data_length);

    // Copio el path
    memcpy(packet->data, payload->path, path_lenght);

    // Donde termina el path, copio el modo
    memcpy(packet->data + path_lenght, &(payload->fileMode), sizeof(mode_t));

    // No hago free del path porque es el que me paso FUSE
    // FIXME: no deberia hacer strcpy del path para independizarme de FUSE?
    free(payload);

    return packet;
}

struct nipc_create* deserialize_create(struct nipc_packet* packet) {
    if (packet->type != nipc_create) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_create* instance = empty_nipc_create();
    size_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->fileMode), packet->data + path_length, sizeof(mode_t));
    free(packet->data);
    free(packet); // FIXME: hago el free aca?
    return instance;
}

struct nipc_create* empty_nipc_create() {
    struct nipc_create* instance = malloc(sizeof(struct nipc_create));
    instance->nipcType = nipc_create;
    instance->serialize = &serialize_create;
//    instance->deserealize = &deserialize_create;
    return instance;
}

struct nipc_create* new_nipc_create(const char* path, mode_t mode) {
    struct nipc_create* instance = empty_nipc_create();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->fileMode = mode;
    return instance;
}

static struct nipc_packet* serialize_open(struct nipc_open* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->flags);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->flags),
            sizeof(payload->flags));

    return packet;
}

struct nipc_open* deserialize_open(struct nipc_packet* packet) {
    if (packet->type != nipc_open) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_open* instance = empty_nipc_open();
    size_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->flags), packet->data + path_length,
            sizeof(instance->flags));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_open* empty_nipc_open() {
    struct nipc_open* instance = malloc(sizeof(struct nipc_open));
    instance->nipcType = nipc_open;
    instance->serialize = &serialize_open;
    return instance;
}

struct nipc_open* new_nipc_open(const char* path, int flags) {
    struct nipc_open* instance = empty_nipc_open();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->flags = flags;
    return instance;
}

static struct nipc_packet* serialize_read(struct nipc_read* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->size)
            + sizeof(payload->offset);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->size), sizeof(payload->size));

    memcpy(packet->data + path_lenght + sizeof(payload->size),
            &(payload->offset), sizeof(payload->offset));

    return packet;
}

struct nipc_read* deserialize_read(struct nipc_packet* packet) {
    if (packet->type != nipc_read) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_read* instance = empty_nipc_read();
    size_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->size), packet->data + path_length,
            sizeof(instance->size));
    memcpy(&(instance->offset),
            packet->data + path_length + sizeof(instance->size),
            sizeof(instance->offset));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_read* empty_nipc_read() {
    struct nipc_read* instance = malloc(sizeof(struct nipc_read));
    instance->nipcType = nipc_read;
    instance->serialize = &serialize_read;
    return instance;
}

struct nipc_read* new_nipc_read(const char* path, size_t size, off_t offset) {
    struct nipc_read* instance = empty_nipc_read();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->size = size;
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_write(struct nipc_write* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + payload->size + sizeof(payload->size)
            + sizeof(payload->offset);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->size), sizeof(payload->size));

    memcpy(packet->data + path_lenght + sizeof(payload->size), payload->data,
            payload->size);

    memcpy(packet->data + path_lenght + sizeof(payload->size) + payload->size,
            &(payload->offset), sizeof(payload->offset));

    return packet;
}

struct nipc_write* deserialize_write(struct nipc_packet* packet) {
    if (packet->type != nipc_write) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_write* instance = empty_nipc_write();
    size_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->size), packet->data + path_length,
            sizeof(instance->size));
    instance->data = malloc(instance->size);
    strcpy(instance->data, packet->data + path_length + sizeof(instance->size));
    memcpy(&(instance->offset),
            packet->data + path_length + sizeof(instance->size)
                    + instance->size, sizeof(instance->offset));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_write* empty_nipc_write() {
    struct nipc_write* instance = malloc(sizeof(struct nipc_write));
    instance->nipcType = nipc_write;
    instance->serialize = &serialize_write;
    return instance;
}

struct nipc_write* new_nipc_write(const char* path, const char* data,
        size_t size, off_t offset) {
    struct nipc_write* instance = empty_nipc_write();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->data = malloc(strlen(data) + 1);
    strcpy(instance->data, data);
    instance->size = size;
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_unlink(struct nipc_unlink* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    packet->type = payload->nipcType;
    packet->data_length = strlen(payload->path) + 1;
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, packet->data_length);

    return packet;
}

struct nipc_unlink* deserialize_unlink(struct nipc_packet* packet) {
    if (packet->type != nipc_unlink) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_unlink* instance = empty_nipc_unlink();
    instance->path = malloc(packet->data_length);
    strcpy(instance->path, packet->data);
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_unlink* empty_nipc_unlink() {
    struct nipc_unlink* instance = malloc(sizeof(struct nipc_unlink));
    instance->nipcType = nipc_unlink;
    instance->serialize = &serialize_unlink;
    return instance;
}

struct nipc_unlink* new_nipc_unlink(const char* path) {
    struct nipc_unlink* instance = empty_nipc_unlink();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_release(struct nipc_release* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->flags);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->flags),
            sizeof(payload->flags));

    return packet;
}

struct nipc_release* deserialize_release(struct nipc_packet* packet) {
    if (packet->type != nipc_release) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_release* instance = empty_nipc_release();
    size_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->flags), packet->data + path_length,
            sizeof(instance->flags));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_release* empty_nipc_release() {
    struct nipc_release* instance = malloc(sizeof(struct nipc_release));
    instance->nipcType = nipc_release;
    instance->serialize = &serialize_release;
    return instance;
}

struct nipc_release* new_nipc_release(const char* path, int flags) {
    struct nipc_release* instance = empty_nipc_release();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->flags = flags;
    return instance;
}

static struct nipc_packet* serialize_mkdir(struct nipc_mkdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(mode_t);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->fileMode), sizeof(mode_t));

    free(payload);

    return packet;
}

struct nipc_mkdir* deserialize_mkdir(struct nipc_packet* packet) {
    if (packet->type != nipc_mkdir) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_mkdir* instance = empty_nipc_mkdir();
    size_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->fileMode), packet->data + path_length, sizeof(mode_t));
    free(packet->data);
    free(packet); // FIXME: hago el free aca?
    return instance;
}

struct nipc_mkdir* empty_nipc_mkdir() {
    struct nipc_mkdir* instance = malloc(sizeof(struct nipc_mkdir));
    instance->nipcType = nipc_mkdir;
    instance->serialize = &serialize_mkdir;
    return instance;
}

struct nipc_mkdir* new_nipc_mkdir(const char* path, mode_t mode) {
    struct nipc_mkdir* instance = empty_nipc_mkdir();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->fileMode = mode;
    return instance;
}

static struct nipc_packet* serialize_rmdir(struct nipc_rmdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    packet->type = payload->nipcType;
    packet->data_length = strlen(payload->path) + 1;
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, packet->data_length);

    return packet;
}

struct nipc_rmdir* deserialize_rmdir(struct nipc_packet* packet) {
    if (packet->type != nipc_rmdir) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_rmdir* instance = empty_nipc_rmdir();
    instance->path = malloc(packet->data_length);
    strcpy(instance->path, packet->data);
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_rmdir* empty_nipc_rmdir() {
    struct nipc_rmdir* instance = malloc(sizeof(struct nipc_rmdir));
    instance->nipcType = nipc_rmdir;
    instance->serialize = &serialize_rmdir;
    return instance;
}

struct nipc_rmdir* new_nipc_rmdir(const char* path) {
    struct nipc_rmdir* instance = empty_nipc_rmdir();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_readdir(struct nipc_readdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(payload->offset);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->offset),
            sizeof(payload->offset));

    free(payload);

    return packet;
}

struct nipc_readdir* deserialize_readdir(struct nipc_packet* packet) {
    if (packet->type != nipc_readdir) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_readdir* instance = empty_nipc_readdir();
    size_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->offset), packet->data + path_length,
            sizeof(instance->offset));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_readdir* empty_nipc_readdir() {
    struct nipc_readdir* instance = malloc(sizeof(struct nipc_readdir));
    instance->nipcType = nipc_readdir;
    instance->serialize = &serialize_readdir;
    return instance;
}

struct nipc_readdir* new_nipc_readdir(const char* path, off_t offset) {
    struct nipc_readdir* instance = empty_nipc_readdir();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_getattr(struct nipc_getattr* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->data_length = strlen(payload->path) + 1;
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, packet->data_length);

    free(payload);

    return packet;
}

struct nipc_getattr* deserialize_getattr(struct nipc_packet* packet) {
    if (packet->type != nipc_getattr) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_getattr* instance = empty_nipc_getattr();
    instance->path = malloc(packet->data_length);
    strcpy(instance->path, packet->data);
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_getattr* empty_nipc_getattr() {
    struct nipc_getattr* instance = malloc(sizeof(struct nipc_getattr));
    instance->nipcType = nipc_getattr;
    instance->serialize = &serialize_getattr;
    return instance;
}

struct nipc_getattr* new_nipc_getattr(const char* path) {
    struct nipc_getattr* instance = empty_nipc_getattr();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_truncate(struct nipc_truncate* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(payload->offset);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->offset),
            sizeof(payload->offset));

    free(payload);

    return packet;
}

struct nipc_truncate* deserialize_truncate(struct nipc_packet* packet) {
    if (packet->type != nipc_truncate) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_truncate* instance = empty_nipc_truncate();
    size_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->offset), packet->data + path_length,
            sizeof(instance->offset));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_truncate* empty_nipc_truncate() {
    struct nipc_truncate* instance = malloc(sizeof(struct nipc_truncate));
    instance->nipcType = nipc_truncate;
    instance->serialize = &serialize_truncate;
    return instance;
}

struct nipc_truncate* new_nipc_truncate(const char* path, off_t offset) {
    struct nipc_truncate* instance = empty_nipc_truncate();
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->offset = offset;
    return instance;
}

size_t nipc_serialize(struct nipc_packet *packet, void **rawPacket) {
    size_t typeLenght = sizeof(packet->type);
    size_t dataLengthLength = sizeof(packet->data_length);
    size_t packetSize = typeLenght + dataLengthLength + packet->data_length;
    *rawPacket = malloc(packetSize);
    memcpy(*rawPacket, &packet->type, typeLenght);
    memcpy(*rawPacket + typeLenght, &packet->data_length, dataLengthLength);
    if(packet->data_length) {
        memcpy(*rawPacket + typeLenght + dataLengthLength, packet->data,
            packet->data_length);
    }
    // FIXME: free(packet->data);
    free(packet);
    return packetSize;
}

struct nipc_packet *nipc_deserialize(void *rawPacket, size_t packetSize) {
    struct nipc_packet *packet = malloc(sizeof(struct nipc_packet));
    memcpy(&(packet->type), rawPacket, sizeof(packet->type));
    memcpy(&(packet->data_length), rawPacket + sizeof(packet->type), sizeof(packet->data_length));
    packet->data = malloc(packet->data_length);
    if(packet->data_length) {
        memcpy(packet->data, rawPacket + sizeof(packet->type) + sizeof(packet->data_length), packet->data_length);
    } else {
        packet->data = NULL;
    }
    free(rawPacket);
    return packet;
}

struct nipc_packet* new_nipc_error(char *errorMessage) {
    struct nipc_packet *instance = malloc(sizeof(struct nipc_packet));
    instance->type = nipc_error;
    instance->data = errorMessage;
    instance->data_length = strlen(errorMessage);
    return instance;
}

struct nipc_packet* new_nipc_ok() {
    struct nipc_packet *instance = malloc(sizeof(struct nipc_packet));
    instance->type = nipc_ok;
    instance->data = NULL;
    instance->data_length = 0;
    return instance;
}






struct nipc_packet *new_nipc_read_response(void *data, size_t dataLength) {
    struct nipc_packet *instance = malloc(sizeof(struct nipc_packet));
    instance->type = nipc_read_response;
    instance->data = data;
    instance->data_length = dataLength;
    return instance;
}




static struct nipc_packet* serialize_readdir_response(struct nipc_readdir_response *payload) {
    /**
     * Cuando escribi esto, solo Dios y yo lo entendiamos.
     *
     * Ahora es solo el.
     */
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    int index;
    void **serializedEntries = calloc(payload->entriesLength, sizeof(void *));
    size_t entriesSize = 0; // entriesSize es el tamanio de todas las entradas (bytes), entriesLength es la cantidad de entradas
    void *serializedEntry = NULL;

    for(index = 0; index < payload->entriesLength; index++) {
        struct readdir_entry *entry = &(payload->entries[index]);
        size_t entrySize = sizeof(entrySize) + sizeof(entry->mode) + sizeof(entry->n_link) + sizeof(entry->size) + strlen(entry->path) + 1;

        serializedEntry = malloc(entrySize);
        serializedEntries[index] = serializedEntry;

        memcpy(serializedEntry, &entrySize, sizeof(entrySize));
        serializedEntry += sizeof(entrySize);
        memcpy(serializedEntry, &entry->mode, sizeof(entry->mode));
        serializedEntry += sizeof(entry->mode);
        memcpy(serializedEntry, &entry->n_link, sizeof(entry->n_link));
        serializedEntry += sizeof(entry->n_link);
        memcpy(serializedEntry, &entry->size, sizeof(entry->size));
        serializedEntry += sizeof(entry->size);
        memcpy(serializedEntry, entry->path, strlen(entry->path) + 1);
        serializedEntry += strlen(entry->path) + 1;
        entriesSize += entrySize;
    }

    packet->type = payload->nipcType;
    packet->data_length = entriesSize + sizeof(payload->entriesLength);
    packet->data = malloc(packet->data_length);
    void *stream = packet->data;
    memcpy(stream, &(payload->entriesLength), sizeof(payload->entriesLength));
    stream += sizeof(payload->entriesLength);

    for(index = 0; index < payload->entriesLength; index++) {
        serializedEntry = serializedEntries[index];
        size_t entrySize = *((size_t *)serializedEntry);
        memcpy(stream, serializedEntry, entrySize);
        stream += entrySize;
    }

    return packet;
}

struct nipc_readdir_response *deserialize_readdir_response(struct nipc_packet *packet) {
    if (packet->type != nipc_readdir_response) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_readdir_response* instance = empty_nipc_readdir_response();
    memcpy(&(instance->entriesLength), packet->data, sizeof(instance->entriesLength));
    instance->entries = calloc(instance->entriesLength, sizeof(struct readdir_entry));

    int index;
    size_t entrySize;
    struct readdir_entry *entry;

    void *stream = packet->data + sizeof(instance->entriesLength);
    int entrySizeSize = sizeof(entrySize);
    size_t nonPathEntrySize = sizeof(entry->mode) + sizeof(entry->n_link) + sizeof(entry->size) + entrySizeSize;


    for(index = 0; index < instance->entriesLength; index++) {
        entrySize = *((size_t *)stream);
        entry = &(instance->entries[index]);
        stream += entrySizeSize;
        memcpy(&(entry->mode), stream, sizeof(entry->mode));
        stream += sizeof(entry->mode);
        memcpy(&(entry->n_link), stream, sizeof(entry->n_link));
        stream += sizeof(entry->n_link);
        memcpy(&(entry->size), stream, sizeof(entry->size));
        stream += sizeof(entry->size);
        size_t pathLength = entrySize - nonPathEntrySize;
        entry->path = malloc(pathLength);
        memcpy(entry->path, stream, pathLength);
        stream += pathLength;
    }

    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_readdir_response* empty_nipc_readdir_response() {
    struct nipc_readdir_response* instance = malloc(sizeof(struct nipc_readdir_response));
    instance->nipcType = nipc_readdir_response;
    instance->serialize = &serialize_readdir_response;
    return instance;
}

struct nipc_readdir_response* new_nipc_readdir_response(u_int32_t entriesLength, struct readdir_entry *entries) {
    struct nipc_readdir_response* instance = empty_nipc_readdir_response();
    instance->entriesLength = entriesLength;
    instance->entries = entries;
    return instance;
}





static struct nipc_packet* serialize_getattr_response(struct nipc_getattr_response *payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;

    packet->data_length = sizeof(payload->entry->mode) + sizeof(payload->entry->n_link) + sizeof(payload->entry->size);
    packet->data = malloc(packet->data_length);

    void *data = packet->data;

    memcpy(data, &(payload->entry->mode), sizeof(payload->entry->mode));
    data += sizeof(payload->entry->mode);

    memcpy(data, &(payload->entry->n_link), sizeof(payload->entry->n_link));
    data += sizeof(payload->entry->n_link);

    memcpy(data, &(payload->entry->size), sizeof(payload->entry->n_link));

    free(payload->entry);
    free(payload);

    return packet;
}

struct nipc_getattr_response* empty_nipc_getattr_response() {
    struct nipc_getattr_response *instance = malloc(sizeof(struct nipc_getattr_response));
    instance->nipcType = nipc_getattr_response;
    instance->serialize = &serialize_getattr_response;
    return instance;
}

struct nipc_getattr_response *deserialize_getattr_response(struct nipc_packet* packet) {
    if (packet->type != nipc_getattr_response) {
        // FIXME: agregar todos los return nipc_error() despues de perror
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_getattr_response* instance = empty_nipc_getattr_response();
    instance->entry = malloc(sizeof(struct readdir_entry));
    void *data = packet->data;

    memcpy(&(instance->entry->mode), data, sizeof(instance->entry->mode));
    data += sizeof(instance->entry->mode);

    memcpy(&(instance->entry->n_link), data, sizeof(instance->entry->n_link));
    data += sizeof(instance->entry->n_link);

    memcpy(&(instance->entry->size), data, sizeof(instance->entry->size));

    free(packet->data);
    free(packet);

    return instance;
}

struct nipc_getattr_response* new_nipc_getattr_response(struct readdir_entry *entry) {
    struct nipc_getattr_response *response = empty_nipc_getattr_response();
    response->entry = entry;
    return response;
}

struct nipc_packet *new_getattr_error(int errorNumber) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    packet->type = nipc_getattr_error;
    packet->data_length = sizeof(errorNumber);
    packet->data = malloc(packet->data_length);

    memcpy(packet->data, &errorNumber, sizeof(errorNumber));

    return packet;
}

struct nipc_packet *new_nipc_handshake(char *message) {
    struct nipc_packet *packet = malloc(sizeof(struct nipc_packet));
    packet->type = nipc_handshake;
    packet->data_length = strlen(message) + 1;
    packet->data = malloc(packet->data_length);
    strcpy(packet->data, message);
    return packet;
}

struct nipc_packet *new_nipc_handshake_hello() {
    return new_nipc_handshake(HANDSHAKE_HELLO);
}

struct nipc_packet *new_nipc_handshake_ok() {
    return new_nipc_handshake(HANDSHAKE_OK);
}


struct nipc_packet *new_nipc_disconnected() {
    struct nipc_packet *packet = malloc(sizeof(struct nipc_packet));
    packet->type = nipc_disconnected;
    packet->data_length = 0;
    packet->data = NULL;
    return packet;
}
