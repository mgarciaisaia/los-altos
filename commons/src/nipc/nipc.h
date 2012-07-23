#ifndef NIPC_H_
#define NIPC_H_
#include <sys/types.h>

#define HANDSHAKE_HELLO "Estan listos, chicos?"
#define HANDSHAKE_OK "Si, mi capitan, estamos listos!"

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
    nipc_truncate,
    nipc_error,
    nipc_ok,
    nipc_read_response,
    nipc_readdir_response,
    nipc_getattr_response,
    nipc_getattr_error,
    nipc_handshake,
    nipc_disconnected
};

char *nombre_del_enum_nipc(enum tipo_nipc tipo);

struct nipc_packet {
    enum tipo_nipc type;
    u_int16_t data_length;
    u_int32_t client_id;
    void* data;
} __attribute__ ((__packed__));

struct nipc_create {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_create*);
    void* path;
    mode_t fileMode;
};

struct nipc_create* deserialize_create(struct nipc_packet* packet);

struct nipc_create* empty_nipc_create();

//XXX: manejar flags?
struct nipc_create* new_nipc_create(u_int32_t client_id, const char* path, mode_t mode);



struct nipc_open {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_open*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_open* deserialize_open(struct nipc_packet* packet);

struct nipc_open* empty_nipc_open();

struct nipc_open* new_nipc_open(u_int32_t client_id, const char* path, int flags);



struct nipc_read {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_read*);
    char* path;
    size_t size;
    off_t offset;
};

struct nipc_read* deserialize_read(struct nipc_packet* packet);

struct nipc_read* empty_nipc_read();

struct nipc_read* new_nipc_read(u_int32_t client_id, const char* path, size_t size, off_t offset);




struct nipc_write {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_write*);
    char* path;
    char* data;
    size_t size;
    off_t offset;
};

struct nipc_write* deserialize_write(struct nipc_packet* packet);

struct nipc_write* empty_nipc_write();

struct nipc_write* new_nipc_write(u_int32_t client_id, const char* path, const char* data, size_t size, off_t offset);




struct nipc_unlink {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_unlink*);
    char* path;
};

struct nipc_unlink* deserialize_unlink(struct nipc_packet* packet);

struct nipc_unlink* empty_nipc_unlink();

struct nipc_unlink* new_nipc_unlink(u_int32_t client_id, const char* path);




struct nipc_release {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_release*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_release* deserialize_release(struct nipc_packet* packet);

struct nipc_release* empty_nipc_release();

struct nipc_release* new_nipc_release(u_int32_t client_id, const char* path, int flags);






struct nipc_mkdir {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_mkdir*);
    char* path;
    mode_t fileMode;
};

struct nipc_mkdir* deserialize_mkdir(struct nipc_packet* packet);

struct nipc_mkdir* empty_nipc_mkdir();

struct nipc_mkdir* new_nipc_mkdir(u_int32_t client_id, const char* path, mode_t mode);





struct nipc_rmdir {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_rmdir*);
    char* path;
};

struct nipc_rmdir* deserialize_rmdir(struct nipc_packet* packet);

struct nipc_rmdir* empty_nipc_rmdir();

struct nipc_rmdir* new_nipc_rmdir(u_int32_t client_id, const char* path);







struct nipc_readdir {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_readdir*);
    char* path;
    off_t offset;
};

struct nipc_readdir* deserialize_readdir(struct nipc_packet* packet);

struct nipc_readdir* empty_nipc_readdir();

struct nipc_readdir* new_nipc_readdir(u_int32_t client_id, const char* path, off_t offset);










struct nipc_getattr {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_getattr*);
    char* path;
};

struct nipc_getattr* deserialize_getattr(struct nipc_packet* packet);

struct nipc_getattr* empty_nipc_getattr();

struct nipc_getattr* new_nipc_getattr(u_int32_t client_id, const char* path);








struct nipc_truncate {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_truncate*);
    char* path;
    off_t offset;
};

struct nipc_truncate* deserialize_truncate(struct nipc_packet* packet);

struct nipc_truncate* empty_nipc_truncate();

struct nipc_truncate* new_nipc_truncate(u_int32_t client_id, const char* path, off_t offset);




size_t nipc_serialize(struct nipc_packet *packet, void **rawPacket);

struct nipc_packet *nipc_deserialize(void *rawPacket, size_t packetSize);

struct nipc_packet* new_nipc_error(char *errorMessage);

struct nipc_packet* new_nipc_ok(u_int32_t client_id);





struct nipc_packet *new_nipc_read_response(void *data, size_t dataLength, u_int32_t client_id);



struct readdir_entry {
    char *path;
    __mode_t mode;
    __off_t size;
    __nlink_t n_link;
};

void readdir_entry_destroy(void *target);

struct nipc_readdir_response {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_readdir_response*);
    u_int32_t entriesLength;
    struct readdir_entry *entries;
};

struct nipc_readdir_response* deserialize_readdir_response(struct nipc_packet* packet);

struct nipc_readdir_response* empty_nipc_readdir_response();

struct nipc_readdir_response* new_nipc_readdir_response(u_int32_t entriesLength, struct readdir_entry *entries, u_int32_t client_id);



struct nipc_getattr_response {
    enum tipo_nipc nipcType;
    u_int32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_getattr_response*);
    struct readdir_entry *entry;
};

struct nipc_getattr_response *deserialize_getattr_response(struct nipc_packet* packet);

struct nipc_getattr_response* new_nipc_getattr_response(struct readdir_entry *entry, u_int32_t client_id);

struct nipc_packet *new_getattr_error(int errorNumber, u_int32_t client_id);

struct nipc_packet *new_nipc_handshake_hello();

struct nipc_packet *new_nipc_handshake_ok();

struct nipc_packet *new_nipc_disconnected();

#endif /* NIPC_H_ */
