#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nipc.h"
#include <stdint.h>

static char *enum_nipc_names[19] = {"CREATE", "OPEN", "READ", "WRITE", "UNLINK", "RELEASE", "MKDIR", "RMDIR", "READDIR", "GETATTR", "TRUNCATE", "ERROR", "OK", "READ_RESPONSE", "READDIR_RESPONSE", "GETATTR_RESPONSE", "HANDSHAKE"};

char *nombre_del_enum_nipc(enum tipo_nipc tipo) {
    if(tipo>19) {
        return "TIPO_INVALIDO";
    }
    return enum_nipc_names[tipo];
}

static struct nipc_packet* serialize_create(struct nipc_create* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1; //(el +1 es para el \0)

    // El largo de los datos es el largo del path + lo que ocupa el modo
    packet->data_length = path_lenght + sizeof(uint32_t);

    packet->data = malloc(packet->data_length);

    // Copio el path
    memcpy(packet->data, payload->path, path_lenght);

    // Donde termina el path, copio el modo
    memcpy(packet->data + path_lenght, &(payload->fileMode), sizeof(uint32_t));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_create* deserialize_create(struct nipc_packet* packet) {
    if (packet->type != nipc_create) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_create* instance = empty_nipc_create();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->fileMode), packet->data + path_length, sizeof(uint32_t));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_create* empty_nipc_create() {
    struct nipc_create* instance = malloc(sizeof(struct nipc_create));
    instance->nipcType = nipc_create;
    instance->serialize = &serialize_create;
//    instance->deserealize = &deserialize_create;
    return instance;
}

struct nipc_create* new_nipc_create(uint32_t client_id, const char* path, uint32_t mode) {
    struct nipc_create* instance = empty_nipc_create();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->fileMode = mode;
    return instance;
}

static struct nipc_packet* serialize_open(struct nipc_open* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->flags);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->flags),
            sizeof(payload->flags));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_open* deserialize_open(struct nipc_packet* packet) {
    if (packet->type != nipc_open) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_open* instance = empty_nipc_open();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen(packet->data) + 1;
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

struct nipc_open* new_nipc_open(uint32_t client_id, const char* path, int flags) {
    struct nipc_open* instance = empty_nipc_open();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->flags = flags;
    return instance;
}

static struct nipc_packet* serialize_read(struct nipc_read* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->size)
            + sizeof(payload->offset);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->size), sizeof(payload->size));

    memcpy(packet->data + path_lenght + sizeof(payload->size),
            &(payload->offset), sizeof(payload->offset));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_read* deserialize_read(struct nipc_packet* packet) {
    if (packet->type != nipc_read) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_read* instance = empty_nipc_read();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen(packet->data) + 1;
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

struct nipc_read* new_nipc_read(uint32_t client_id, const char* path, uint32_t size, uint32_t offset) {
    struct nipc_read* instance = empty_nipc_read();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->size = size;
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_write(struct nipc_write* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + payload->size + sizeof(payload->size) + sizeof(payload->offset);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->size), sizeof(payload->size));

    memcpy(packet->data + path_lenght + sizeof(payload->size), payload->data, payload->size);

    memcpy(packet->data + path_lenght + sizeof(payload->size) + payload->size, &(payload->offset), sizeof(payload->offset));

    free(payload->data);
    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_write* deserialize_write(struct nipc_packet* packet) {
    if (packet->type != nipc_write) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_write* instance = empty_nipc_write();
    instance->client_id = packet->client_id;

    uint32_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->size), packet->data + path_length, sizeof(instance->size));
    instance->data = malloc(instance->size);
    memcpy(instance->data, packet->data + path_length + sizeof(instance->size), instance->size);
    memcpy(&(instance->offset), packet->data + path_length + sizeof(instance->size) + instance->size, sizeof(instance->offset));
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

struct nipc_write* new_nipc_write(uint32_t client_id, const char* path,
        const char* data, uint32_t size, uint32_t offset) {
    struct nipc_write* instance = empty_nipc_write();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->data = malloc(size);
    memcpy(instance->data, data, size);
    instance->size = size;
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_unlink(struct nipc_unlink* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
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
    instance->client_id = packet->client_id;
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

struct nipc_unlink* new_nipc_unlink(uint32_t client_id, const char* path) {
    struct nipc_unlink* instance = empty_nipc_unlink();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_release(struct nipc_release* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;

    packet->data_length = path_lenght + sizeof(payload->flags);

    packet->data = malloc(packet->data_length);

    memcpy(packet->data, payload->path, path_lenght);

    memcpy(packet->data + path_lenght, &(payload->flags),
            sizeof(payload->flags));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_release* deserialize_release(struct nipc_packet* packet) {
    if (packet->type != nipc_release) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_release* instance = empty_nipc_release();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen(packet->data) + 1;
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

struct nipc_release* new_nipc_release(uint32_t client_id, const char* path, int flags) {
    struct nipc_release* instance = empty_nipc_release();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->flags = flags;
    return instance;
}

static struct nipc_packet* serialize_mkdir(struct nipc_mkdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(uint32_t);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->fileMode), sizeof(uint32_t));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_mkdir* deserialize_mkdir(struct nipc_packet* packet) {
    if (packet->type != nipc_mkdir) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_mkdir* instance = empty_nipc_mkdir();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen((char*) packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->fileMode), packet->data + path_length, sizeof(uint32_t));
    free(packet->data);
    free(packet);
    return instance;
}

struct nipc_mkdir* empty_nipc_mkdir() {
    struct nipc_mkdir* instance = malloc(sizeof(struct nipc_mkdir));
    instance->nipcType = nipc_mkdir;
    instance->serialize = &serialize_mkdir;
    return instance;
}

struct nipc_mkdir* new_nipc_mkdir(uint32_t client_id, const char* path, uint32_t mode) {
    struct nipc_mkdir* instance = empty_nipc_mkdir();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->fileMode = mode;
    return instance;
}

static struct nipc_packet* serialize_rmdir(struct nipc_rmdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));

    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
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
    instance->client_id = packet->client_id;
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

struct nipc_rmdir* new_nipc_rmdir(uint32_t client_id, const char* path) {
    struct nipc_rmdir* instance = empty_nipc_rmdir();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_readdir(struct nipc_readdir* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(payload->offset);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->offset),
            sizeof(payload->offset));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_readdir* deserialize_readdir(struct nipc_packet* packet) {
    if (packet->type != nipc_readdir) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_readdir* instance = empty_nipc_readdir();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen((char*) packet->data) + 1;
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

struct nipc_readdir* new_nipc_readdir(uint32_t client_id, const char* path, uint32_t offset) {
    struct nipc_readdir* instance = empty_nipc_readdir();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->offset = offset;
    return instance;
}

static struct nipc_packet* serialize_getattr(struct nipc_getattr* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    packet->data_length = strlen(payload->path) + 1;
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, packet->data_length);

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_getattr* deserialize_getattr(struct nipc_packet* packet) {
    if (packet->type != nipc_getattr) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_getattr* instance = empty_nipc_getattr();
    instance->client_id = packet->client_id;
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

struct nipc_getattr* new_nipc_getattr(uint32_t client_id, const char* path) {
    struct nipc_getattr* instance = empty_nipc_getattr();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    return instance;
}

static struct nipc_packet* serialize_truncate(struct nipc_truncate* payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    int path_lenght = strlen(payload->path) + 1;
    packet->data_length = path_lenght + sizeof(payload->offset);
    packet->data = malloc(packet->data_length);
    memcpy(packet->data, payload->path, path_lenght);
    memcpy(packet->data + path_lenght, &(payload->offset),
            sizeof(payload->offset));

    free(payload->path);
    free(payload);

    return packet;
}

struct nipc_truncate* deserialize_truncate(struct nipc_packet* packet) {
    if (packet->type != nipc_truncate) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_truncate* instance = empty_nipc_truncate();
    instance->client_id = packet->client_id;
    uint32_t path_length = strlen((char*) packet->data) + 1;
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

struct nipc_truncate* new_nipc_truncate(uint32_t client_id, const char* path, uint32_t offset) {
    struct nipc_truncate* instance = empty_nipc_truncate();
    instance->client_id = client_id;
    instance->path = malloc(strlen(path) + 1);
    strcpy(instance->path, path);
    instance->offset = offset;
    return instance;
}

uint32_t nipc_serialize(struct nipc_packet *packet, void **rawPacket) {
    uint32_t typeLenght = sizeof(packet->type);
    uint32_t dataLengthLength = sizeof(packet->data_length);
    uint32_t clientIdSize = sizeof(packet->client_id);
    uint32_t packetSize = typeLenght + dataLengthLength + clientIdSize + packet->data_length;
    *rawPacket = malloc(packetSize);
    memcpy(*rawPacket, &packet->type, typeLenght);
    memcpy(*rawPacket + typeLenght, &packet->client_id, clientIdSize);
    memcpy(*rawPacket + typeLenght + clientIdSize, &packet->data_length, dataLengthLength);
    if(packet->data_length) {
        memcpy(*rawPacket + typeLenght + clientIdSize + dataLengthLength, packet->data,
            packet->data_length);
    }
    free(packet->data);
    free(packet);
    return packetSize;
}

struct nipc_packet *nipc_deserialize(void *rawPacket, uint32_t packetSize) {
    struct nipc_packet *packet = malloc(sizeof(struct nipc_packet));
    memcpy(&(packet->type), rawPacket, sizeof(packet->type));
    memcpy(&(packet->client_id), rawPacket + sizeof(packet->type), sizeof(packet->client_id));
    memcpy(&(packet->data_length), rawPacket + sizeof(packet->type) + sizeof(packet->client_id), sizeof(packet->data_length));
    packet->data = malloc(packet->data_length);
    if(packet->data_length) {
        memcpy(packet->data, rawPacket + sizeof(packet->type) + sizeof(packet->client_id) + sizeof(packet->data_length), packet->data_length);
    } else {
        packet->data = NULL;
    }
    free(rawPacket);
    return packet;
}







struct nipc_packet *new_nipc_read_response(void *data, uint32_t dataLength, uint32_t client_id) {
    struct nipc_packet *instance = malloc(sizeof(struct nipc_packet));
    instance->type = nipc_read_response;
    instance->client_id = client_id;
    instance->data = data;
    instance->data_length = dataLength;
    return instance;
}



void readdir_entry_destroy(void *target) {
    struct readdir_entry *entry = (struct readdir_entry *)target;
    free(entry->path);
    free(entry);
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
    uint32_t entriesSize = 0; // entriesSize es el tamanio de todas las entradas (bytes), entriesLength es la cantidad de entradas
    void *serializedEntry = NULL;

    for(index = 0; index < payload->entriesLength; index++) {
        struct readdir_entry *entry = &(payload->entries[index]);
        uint32_t entrySize = sizeof(entrySize) + sizeof(entry->mode) + sizeof(entry->n_link) + sizeof(entry->size) + strlen(entry->path) + 1;

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
    packet->client_id = payload->client_id;
    packet->data_length = entriesSize + sizeof(payload->entriesLength);
    /*
     * FIXME: Validar el overflow de data_length y ver como manejarlo (si hace falta)
     */
    packet->data = malloc(packet->data_length);
    void *stream = packet->data;
    memcpy(stream, &(payload->entriesLength), sizeof(payload->entriesLength));
    stream += sizeof(payload->entriesLength);

    for(index = 0; index < payload->entriesLength; index++) {
        serializedEntry = serializedEntries[index];
        uint32_t entrySize = *((uint32_t *)serializedEntry);
        memcpy(stream, serializedEntry, entrySize);
        stream += entrySize;
        free(serializedEntries[index]);
    }

    free(serializedEntries);

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
    uint32_t entrySize;
    struct readdir_entry *entry;

    void *stream = packet->data + sizeof(instance->entriesLength);
    int entrySizeSize = sizeof(entrySize);
    uint32_t nonPathEntrySize = sizeof(entry->mode) + sizeof(entry->n_link) + sizeof(entry->size) + entrySizeSize;


    for(index = 0; index < instance->entriesLength; index++) {
        entrySize = *((uint32_t *)stream);
        entry = &(instance->entries[index]);
        stream += entrySizeSize;
        memcpy(&(entry->mode), stream, sizeof(entry->mode));
        stream += sizeof(entry->mode);
        memcpy(&(entry->n_link), stream, sizeof(entry->n_link));
        stream += sizeof(entry->n_link);
        memcpy(&(entry->size), stream, sizeof(entry->size));
        stream += sizeof(entry->size);
        uint32_t pathLength = entrySize - nonPathEntrySize;
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

struct nipc_readdir_response* new_nipc_readdir_response(uint32_t entriesLength, struct readdir_entry *entries, uint32_t client_id) {
    struct nipc_readdir_response* instance = empty_nipc_readdir_response();
    instance->entriesLength = entriesLength;
    instance->entries = entries;
    instance->client_id = client_id;
    return instance;
}





static struct nipc_packet* serialize_getattr_response(struct nipc_getattr_response *payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;

    packet->client_id = 0;

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

    instance->entry->path = NULL;

    free(packet->data);
    free(packet);

    return instance;
}

struct nipc_getattr_response* new_nipc_getattr_response(struct readdir_entry *entry, uint32_t client_id) {
    struct nipc_getattr_response *response = empty_nipc_getattr_response();
    response->entry = entry;
    response->client_id = client_id;
    return response;
}

struct nipc_packet *new_nipc_handshake(char *message, uint32_t client_id) {
    struct nipc_packet *packet = malloc(sizeof(struct nipc_packet));
    packet->type = nipc_handshake;
    packet->client_id = client_id;
    packet->data_length = strlen(message) + 1;
    packet->data = malloc(packet->data_length);
    strcpy(packet->data, message);
    return packet;
}

struct nipc_packet *new_nipc_handshake_hello() {
    return new_nipc_handshake(HANDSHAKE_HELLO, 0);
}

struct nipc_packet *new_nipc_handshake_ok(int new_client_id) {
    return new_nipc_handshake(HANDSHAKE_OK, new_client_id);
}




struct nipc_packet* new_nipc_ok(uint32_t client_id) {
    struct nipc_packet *instance = malloc(sizeof(struct nipc_packet));
    instance->type = nipc_ok;
    instance->client_id = client_id;
    instance->data = NULL;
    instance->data_length = 0;
    return instance;
}






static struct nipc_packet *serialize_error(struct nipc_error *payload) {
    struct nipc_packet* packet = malloc(sizeof(struct nipc_packet));
    packet->type = payload->nipcType;
    packet->client_id = payload->client_id;
    uint32_t message_length = strlen(payload->errorMessage);
    packet->data_length = sizeof(payload->errorCode) + sizeof(message_length) + message_length;
    packet->data = malloc(packet->data_length);

    memcpy(packet->data, &payload->errorCode, sizeof(payload->errorCode));
    memcpy(packet->data + sizeof(payload->errorCode), &message_length, sizeof(message_length));
    if(message_length > 0) {
        memcpy(packet->data + sizeof(payload->errorCode) + sizeof(message_length), payload->errorMessage, message_length);
    }

    free(payload->errorMessage);
    free(payload);

    return packet;
}

struct nipc_error *empty_nipc_error() {
    struct nipc_error *instance = malloc(sizeof(struct nipc_error));
    instance->serialize = &serialize_error;
    instance->client_id = 0;
    instance->nipcType = nipc_error;
    instance->errorMessage = strdup("");
    instance->errorCode = 0;
    return instance;
}

struct nipc_error *deserialize_error(struct nipc_packet *packet) {
    if (packet->type != nipc_error) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_error* instance = empty_nipc_error();
    memcpy(&instance->errorCode, packet->data, sizeof(instance->errorCode));
    uint32_t message_length = 0;
    memcpy(&message_length, packet->data + sizeof(instance->errorCode), sizeof(message_length));
    if(message_length > 0) {
        free(instance->errorMessage);
        instance->errorMessage = calloc(1, message_length + 1);
        memcpy(instance->errorMessage, packet->data + sizeof(instance->errorCode) + sizeof(message_length), message_length);
    }
    free(packet->data);
    free(packet);
    return instance;
}


struct nipc_error* new_nipc_error_message(char *errorMessage) {
    struct nipc_error *instance = empty_nipc_error();
    free(instance->errorMessage);
    instance->errorMessage = strdup(errorMessage);
    return instance;
}

struct nipc_error *new_nipc_error_code(uint32_t client_id, int32_t errorNumber) {
    struct nipc_error *instance = empty_nipc_error();
    instance->client_id = client_id;
    instance->errorCode = errorNumber;
    return instance;
}

struct nipc_error *new_nipc_error(int32_t error_code, char *error_message) {
    struct nipc_error *instance = empty_nipc_error();
    instance->errorCode = error_code;
    free(instance->errorMessage);
    instance->errorMessage = strdup(error_message);
    return instance;
}
