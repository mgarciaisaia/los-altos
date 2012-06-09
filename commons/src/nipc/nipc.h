#ifndef NIPC_H_
#define NIPC_H_
#include <sys/types.h>

enum tipo_nipc {
    nipc_create,
    nipc_open,
    nipc_read,
    nipc_write,
    nipc_unlink,
    nipc_release,
    nipc_mkdir,
    nipc_rmdir,
    nipc_readdir,
    nipc_getattr,
    nipc_truncate
};

struct nipc_packet {
    enum tipo_nipc type; // FIXME: deberia ser 1 unico byte?
    u_int16_t data_length;
    void* data;
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
struct nipc_create* new_nipc_create(const char* path, mode_t mode);



struct nipc_open {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_open*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_open* deserialize_open(struct nipc_packet* packet);

struct nipc_open* empty_nipc_open();

struct nipc_open* new_nipc_open(const char* path, int flags);



struct nipc_read {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_read*);
    char* path;
    size_t size;
    off_t offset;
};

struct nipc_read* deserialize_read(struct nipc_packet* packet);

struct nipc_read* empty_nipc_read();

struct nipc_read* new_nipc_read(const char* path, size_t size, off_t offset);




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

struct nipc_write* new_nipc_write(const char* path, const char* data, size_t size, off_t offset);




struct nipc_unlink {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_unlink*);
    char* path;
};

struct nipc_unlink* deserialize_unlink(struct nipc_packet* packet);

struct nipc_unlink* empty_nipc_unlink();

struct nipc_unlink* new_nipc_unlink(const char* path);




struct nipc_release {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_release*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_release* deserialize_release(struct nipc_packet* packet);

struct nipc_release* empty_nipc_release();

struct nipc_release* new_nipc_release(const char* path, int flags);






struct nipc_mkdir {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_mkdir*);
    char* path;
    mode_t fileMode;
};

struct nipc_mkdir* deserialize_mkdir(struct nipc_packet* packet);

struct nipc_mkdir* empty_nipc_mkdir();

struct nipc_mkdir* new_nipc_mkdir(const char* path, mode_t mode);





struct nipc_rmdir {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_rmdir*);
    char* path;
};

struct nipc_rmdir* deserialize_rmdir(struct nipc_packet* packet);

struct nipc_rmdir* empty_nipc_rmdir();

struct nipc_rmdir* new_nipc_rmdir(const char* path);







struct nipc_readdir {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_readdir*);
    char* path;
    off_t offset;
};

struct nipc_readdir* deserialize_readdir(struct nipc_packet* packet);

struct nipc_readdir* empty_nipc_readdir();

struct nipc_readdir* new_nipc_readdir(const char* path, off_t offset);










struct nipc_getattr {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_getattr*);
    char* path;
};

struct nipc_getattr* deserialize_getattr(struct nipc_packet* packet);

struct nipc_getattr* empty_nipc_getattr();

struct nipc_getattr* new_nipc_getattr(const char* path);








struct nipc_truncate {
    enum tipo_nipc nipcType;
    struct nipc_packet* (*serialize)(struct nipc_truncate*);
    char* path;
    off_t offset;
};

struct nipc_truncate* deserialize_truncate(struct nipc_packet* packet);

struct nipc_truncate* empty_nipc_truncate();

struct nipc_truncate* new_nipc_truncate(const char* path, off_t offset);




size_t nipc_serialize(struct nipc_packet *packet, void **rawPacket);

struct nipc_packet *nipc_deserialize(void *rawPacket, size_t packetSize);

#endif /* NIPC_H_ */
