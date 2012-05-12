#include <stdio.h>
#include <stdlib.h>

struct nipc_packet {
    int32_t size;
    char type;
    void* data; // TODO: seguramente tenga que ser un array
};
// TODO: __PACKED?

struct nipc_create {
    char* path;
    int32_t fileMode;
    char nipcType;
    int (*serialize)(struct nipc_create*);
};

static int serialize_create(struct nipc_create* payload) {
    return 4;
}

struct nipc_create* new_nipc_create() {
    struct nipc_create* instance = malloc(sizeof(struct nipc_create));
    instance->nipcType = 'A';
    instance->serialize = &serialize_create;
    return instance;
}

void bleh(struct nipc_create* packet) {
    packet->serialize(packet); //FIXME: serialize tiene que ser puntero a funcion
}
