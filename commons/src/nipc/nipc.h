#ifndef NIPC_H_
#define NIPC_H_
#include <sys/types.h>
#include <stdint.h>

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
    nipc_handshake
};

char *nombre_del_enum_nipc(enum tipo_nipc tipo);

struct nipc_packet {
    enum tipo_nipc type;
    uint32_t data_length;
    uint32_t client_id;
    void* data;
} __attribute__ ((__packed__));

struct nipc_create {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_create*);
    void* path;
    uint32_t fileMode;
};

struct nipc_create* deserialize_create(struct nipc_packet* packet);

struct nipc_create* empty_nipc_create();

//XXX: manejar flags?
struct nipc_create* new_nipc_create(uint32_t client_id, const char* path, uint32_t mode);



struct nipc_open {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_open*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_open* deserialize_open(struct nipc_packet* packet);

struct nipc_open* empty_nipc_open();

struct nipc_open* new_nipc_open(uint32_t client_id, const char* path, int flags);



struct nipc_read {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_read*);
    char* path;
    uint32_t size;
    uint32_t offset;
};

struct nipc_read* deserialize_read(struct nipc_packet* packet);

struct nipc_read* empty_nipc_read();

struct nipc_read* new_nipc_read(uint32_t client_id, const char* path, uint32_t size, uint32_t offset);




struct nipc_write {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_write*);
    char* path;
    char* data;
    uint32_t size;
    uint32_t offset;
};

struct nipc_write* deserialize_write(struct nipc_packet* packet);

struct nipc_write* empty_nipc_write();

struct nipc_write* new_nipc_write(uint32_t client_id, const char* path, const char* data, uint32_t size, uint32_t offset);




struct nipc_unlink {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_unlink*);
    char* path;
};

struct nipc_unlink* deserialize_unlink(struct nipc_packet* packet);

struct nipc_unlink* empty_nipc_unlink();

struct nipc_unlink* new_nipc_unlink(uint32_t client_id, const char* path);




struct nipc_release {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_release*);
    char* path;
    int flags; // FIXME: como se cual de los dos intXX_t usar aca?
};

struct nipc_release* deserialize_release(struct nipc_packet* packet);

struct nipc_release* empty_nipc_release();

struct nipc_release* new_nipc_release(uint32_t client_id, const char* path, int flags);






struct nipc_mkdir {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_mkdir*);
    char* path;
    uint32_t fileMode;
};

struct nipc_mkdir* deserialize_mkdir(struct nipc_packet* packet);

struct nipc_mkdir* empty_nipc_mkdir();

struct nipc_mkdir* new_nipc_mkdir(uint32_t client_id, const char* path, uint32_t mode);





struct nipc_rmdir {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_rmdir*);
    char* path;
};

struct nipc_rmdir* deserialize_rmdir(struct nipc_packet* packet);

struct nipc_rmdir* empty_nipc_rmdir();

struct nipc_rmdir* new_nipc_rmdir(uint32_t client_id, const char* path);







struct nipc_readdir {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_readdir*);
    char* path;
    uint32_t offset;
};

struct nipc_readdir* deserialize_readdir(struct nipc_packet* packet);

struct nipc_readdir* empty_nipc_readdir();

struct nipc_readdir* new_nipc_readdir(uint32_t client_id, const char* path, uint32_t offset);










struct nipc_getattr {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_getattr*);
    char* path;
};

struct nipc_getattr* deserialize_getattr(struct nipc_packet* packet);

struct nipc_getattr* empty_nipc_getattr();

struct nipc_getattr* new_nipc_getattr(uint32_t client_id, const char* path);








struct nipc_truncate {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_truncate*);
    char* path;
    uint32_t offset;
};

struct nipc_truncate* deserialize_truncate(struct nipc_packet* packet);

struct nipc_truncate* empty_nipc_truncate();

struct nipc_truncate* new_nipc_truncate(uint32_t client_id, const char* path, uint32_t offset);




uint32_t nipc_serialize(struct nipc_packet *packet, void **rawPacket);

struct nipc_packet *nipc_deserialize(void *rawPacket, uint32_t packetSize);






struct nipc_packet *new_nipc_read_response(void *data, uint32_t dataLength, uint32_t client_id);



struct readdir_entry {
    char *path;
    uint32_t mode;
    uint32_t size;
    uint32_t n_link;
};

void readdir_entry_destroy(void *target);

struct nipc_readdir_response {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_readdir_response*);
    uint32_t entriesLength;
    struct readdir_entry *entries;
};

struct nipc_readdir_response* deserialize_readdir_response(struct nipc_packet* packet);

struct nipc_readdir_response* empty_nipc_readdir_response();

struct nipc_readdir_response* new_nipc_readdir_response(uint32_t entriesLength, struct readdir_entry *entries, uint32_t client_id);



struct nipc_getattr_response {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_getattr_response*);
    struct readdir_entry *entry;
};

struct nipc_getattr_response *deserialize_getattr_response(struct nipc_packet* packet);

struct nipc_getattr_response* new_nipc_getattr_response(struct readdir_entry *entry, uint32_t client_id);

struct nipc_packet *new_nipc_handshake_hello();

struct nipc_packet *new_nipc_handshake_ok();




struct nipc_packet* new_nipc_ok(uint32_t client_id);





struct nipc_error {
    enum tipo_nipc nipcType;
    uint32_t client_id;
    struct nipc_packet* (*serialize)(struct nipc_error*);
    uint32_t errorCode;
    char *errorMessage;
};

struct nipc_error *deserialize_error(struct nipc_packet *packet);

struct nipc_error* new_nipc_error_message(char *errorMessage);

struct nipc_error *new_nipc_error_code(uint32_t client_id, int32_t error_code);

struct nipc_error *new_nipc_error(int32_t error_code, char *error_message);

#endif /* NIPC_H_ */
