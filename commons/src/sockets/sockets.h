#ifndef SOCKETS_H_
#define SOCKETS_H_
#include <stdint.h>

int socket_connect(char *remoteFileSystemIP, uint16_t port);
size_t socket_query(int socketDescriptor, void *rawRequest, size_t requestSize,
        void *rawResponse);

#endif /* SOCKETS_H_ */
