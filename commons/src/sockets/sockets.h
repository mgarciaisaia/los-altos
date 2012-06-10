#ifndef SOCKETS_H_
#define SOCKETS_H_
#include <stdint.h>

struct nipc_packet *nipc_query(struct nipc_packet *request, char *remoteIP, uint16_t port);
struct nipc_packet *nipc_receive(int socketDescriptor);
int nipc_send(int socketDescriptor, struct nipc_packet *request);

int socket_connect(char *remoteIP, uint16_t port);
int socket_binded(uint16_t port);

#endif /* SOCKETS_H_ */
