#ifndef NIPC_H_
#define NIPC_H_

enum tipo_nipc {
    nipc_create
};

struct nipc_packet {
    enum tipo_nipc type;
    u_int16_t data_length;
    void* data; // TODO: seguramente tenga que ser un array
} __attribute__ ((__packed__));

struct nipc_create {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_create*);
    //struct nipc_create* (*deserealize)(struct nipc_packet*); // FIXME: deserealize es inllamable aca
    char* path;
    mode_t fileMode;
};

static struct nipc_packet* serialize_create(struct nipc_create* payload);

struct nipc_create* deserialize_create(struct nipc_packet* packet);

struct nipc_create* empty_nipc_create();

struct nipc_create* new_nipc_create(char* path, mode_t mode, int flags);

#endif /* NIPC_H_ */
