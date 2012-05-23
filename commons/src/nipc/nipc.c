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
    if(packet->type != nipc_create) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_create* instance = empty_nipc_create();
    size_t path_length = strlen((char*)packet->data) + 1;
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

struct nipc_create* new_nipc_create(char* path, mode_t mode) {
    struct nipc_create* instance = empty_nipc_create();
    instance->path = path;
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

    memcpy(packet->data + path_lenght, &(payload->flags), sizeof(payload->flags));

    return packet;
}

struct nipc_open* deserialize_open(struct nipc_packet* packet) {
    if(packet->type != nipc_open) {
        perror("Error desearilzando paquete - tipo invalido");
    }
    struct nipc_open* instance = empty_nipc_open();
    size_t path_length = strlen(packet->data) + 1;
    instance->path = malloc(path_length);
    strcpy(instance->path, packet->data);
    memcpy(&(instance->flags), packet->data + path_length, sizeof(instance->flags));
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

struct nipc_open* new_nipc_open(char* path, int flags) {
    struct nipc_open* instance = empty_nipc_open();
    instance->path = path;
    instance->flags = flags;
    return instance;
}
