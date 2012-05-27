#ifndef NIPC_H_
#define NIPC_H_

enum tipo_nipc {
    nipc_create,
    nipc_open,
    nipc_read,
    nipc_write
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

struct nipc_create* deserialize_create(struct nipc_packet* packet);

struct nipc_create* empty_nipc_create();

//XXX: manejar flags?
struct nipc_create* new_nipc_create(char* path, mode_t mode);



struct nipc_open {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_open*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_open* deserialize_open(struct nipc_packet* packet);

struct nipc_open* empty_nipc_open();

struct nipc_open* new_nipc_open(char* path, int flags);



struct nipc_read {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_read*);
    char* path;
    size_t size;
    off_t offset;
};

struct nipc_read* deserialize_read(struct nipc_packet* packet);

struct nipc_read* empty_nipc_read();

struct nipc_read* new_nipc_read(char* path, size_t size, off_t offset);




struct nipc_write {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_write*);
    char* path;
    char* data;
    size_t size;
    off_t offset;
};

struct nipc_write* deserialize_write(struct nipc_packet* packet);

struct nipc_write* empty_nipc_write();

struct nipc_write* new_nipc_write(char* path, char* data, size_t size, off_t offset);

#endif /* NIPC_H_ */
