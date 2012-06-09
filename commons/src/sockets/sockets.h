#ifndef SOCKETS_H_
#define SOCKETS_H_
#include <stdint.h>

int socket_connect(char *remoteIP, uint16_t port);
size_t socket_query(int socketDescriptor, void *rawRequest, size_t requestSize,
        void **rawResponse);
int socket_binded(uint16_t port);

#endif /* SOCKETS_H_ */
