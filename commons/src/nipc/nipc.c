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

    //FIXME: hago un free(payload) aca?
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

struct nipc_create* new_nipc_create(char* path, mode_t mode, int flags) {
    struct nipc_create* instance = empty_nipc_create();
    instance->path = path;
    instance->fileMode = mode;
    //FIXME: mandar flags
    return instance;
}
